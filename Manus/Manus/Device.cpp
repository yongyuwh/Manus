/*
Copyright 2015 Manus VR

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "stdafx.h"
#include "Device.h"
#include "ManusMath.h"

#include <hidapi.h>
#include <limits>




// Normalization constants 
#define ACCEL_DIVISOR 16384.0f
#define QUAT_DIVISOR 16384.0f
#define COMPASS_DIVISOR 32.0f
#define FINGER_DIVISOR 255.0f


Device::Device(const char* device_path)
	: m_running(false) {
	size_t len = strlen(device_path) + 1;
	m_device_path = new char[len];
	memcpy(m_device_path, device_path, len * sizeof(char));

	Connect();
}

Device::~Device()
{
	Disconnect();
	delete m_device_path;
}


bool Device::HasDevice(device_type_t device) {
	// TODO
	return false;
}


bool Device::GetData(GLOVE_DATA* data, device_type_t device, unsigned int timeout) {
	uint8_t deviceNr = device - DEVICE_TYPE_LOW;
	// Wait until the thread is done writing a packet
	std::unique_lock<std::mutex> lk(m_report_mutex[deviceNr]);

	// Optionally wait until the next package is sent
	if (timeout > 0)
	{
		m_report_cv[deviceNr].wait_for(lk, std::chrono::milliseconds(timeout));
		if (!m_running)
		{
			lk.unlock();
			return false;
		}
	}

	*data = m_data[deviceNr];

	lk.unlock();


	// Create better return value, check for freshness
	return m_data[deviceNr].PacketNumber > 0;
}

bool Device::GetFlags(uint8_t & flags, device_type_t device, unsigned int timeout) {
	flags = m_report[device - DEVICE_TYPE_LOW].flags;
	return true;
	/*
	// Send request for flags
	m_data_out.device_type = device;
	m_data_out.message_type = MSG_FLAGS_GET;

	// Wait until the thread is done writing a packet
	// But we need to be sure it's sent first
	//std::this_thread::sleep_for(std::chrono::milliseconds(40));
	std::unique_lock<std::mutex> lk(m_flags_mutex);

	// Optionally wait until the next package is sent
	if (timeout > 0)
	{
		m_flags_cv.wait_for(lk, std::chrono::milliseconds(timeout));
		if (!m_running)
		{
			lk.unlock();
			return false;
		}
	}
	

	if (m_flags[device - DEVICE_TYPE_LOW].device_type) {
		flags = m_flags[device - DEVICE_TYPE_LOW].flags;
		m_flags[device - DEVICE_TYPE_LOW].device_type = DEV_NONE;
		return true;
	}
	return false;
	*/
}

bool Device::GetRssi(int32_t &rssi, device_type_t device, unsigned int timeout) {
	rssi = m_report[device - DEVICE_TYPE_LOW].rssi;
	return true;

	/*
	// Send request for stats
	m_data_out.device_type = device;
	m_data_out.message_type = MSG_STATS_GET;

	
	std::unique_lock<std::mutex> lk(m_stats_mutex);
	

	// Optionally wait until the next package is sent
	if (timeout > 0)
	{
		
		m_stats_cv.wait_for(lk, std::chrono::milliseconds(timeout));
		if (!m_running)
		{
			lk.unlock();
			return false;
		}
	}
	

	if (m_rf_stats[device - DEVICE_TYPE_LOW].device_type) {
		rssi = m_rf_stats[device - DEVICE_TYPE_LOW].tx_rssi;
		m_rf_stats[device - DEVICE_TYPE_LOW].device_type = DEV_NONE;
		return true;
	}
	return false;
	*/
}


void Device::SetVibration(float power, device_type_t dev, unsigned int timeout) {
	// clipping
	if (power < 0) power = 0.0f;
	if (power > 1) power = 1.0f;

	m_data_out.device_type = dev;
	m_data_out.message_type = MSG_RUMBLE_PWR;
	m_data_out.rumble.power = (uint16_t)(0xFFFF * power);
}

void Device::SetFlags(uint8_t flags, device_type_t device) {
	m_data_out.device_type = device;
	m_data_out.message_type = MSG_FLAGS_SET;
	m_data_out.flags.flags = flags;
}

void Device::PowerOff(device_type_t device) {
	m_data_out.device_type = device;
	m_data_out.message_type = MSG_POWER_OFF;
}


void Device::Connect() {
	Disconnect();
	m_thread = std::thread(DeviceThread, this);
}

void Device::Disconnect() {
	// Instruct the device thread to stop and
	// wait for it to shut down.
	m_running = false;
	if (m_thread.joinable())
		m_thread.join();
}

void Device::DeviceThread(Device* dev) {
	

	// TODO: remove threading? (HIDAPI can work without, just return old report when hid_read returns 0)
	dev->m_device = hid_open_path(dev->m_device_path);

	if (!dev->m_device)
		return;

	dev->m_running = true;

	// Keep retrieving reports while the SDK is running and the device is connected
	while (dev->m_running && dev->m_device)
	{
		
		if (dev->m_data_out.device_type) {
			USB_OUT_PACKET data;
			data.report_id = 0;
			data.data = dev->m_data_out;
			//int write = hid_write(dev->m_device, (uint8_t*)(&dev->m_data_out), sizeof(m_data_out));
			int write = hid_write(dev->m_device, (uint8_t*)(&data), sizeof(data));
			dev->m_data_out.device_type = DEV_NONE;
		}

		uint8_t report[32];
		int read = hid_read(dev->m_device, report, sizeof(report));

		if (read == -1)
			break;

		{

			if (report[0] == DEVICE_MESSAGE) {
				ESB_DATA_PACKET *recv_data = (ESB_DATA_PACKET *)(1 + report);
				if (recv_data->device_type < DEVICE_TYPE_COUNT + DEVICE_TYPE_LOW) {
					
					uint8_t deviceNr = recv_data->device_type - DEVICE_TYPE_LOW;
					switch (recv_data->message_type) {
					case MSG_FLAGS_GET: {
						std::lock_guard<std::mutex> lk(dev->m_flags_mutex);
						dev->m_flags[deviceNr].device_type = recv_data->device_type;
						dev->m_flags[deviceNr].flags = recv_data->flags.flags;
						dev->m_flags_cv.notify_all();
						break;
						}
					case MSG_STATS_GET: {
						std::lock_guard<std::mutex> lk(dev->m_stats_mutex);
						dev->m_rf_stats[deviceNr].device_type = recv_data->device_type;
						dev->m_rf_stats[deviceNr].tx_rssi = recv_data->rf_stats.tx_rssi;
						dev->m_flags_cv.notify_all();
						
						break;
						}
					}
				}
			} else if (report[0] < DEVICE_TYPE_COUNT + DEVICE_TYPE_LOW) {
				uint8_t deviceNr = report[0] - DEVICE_TYPE_LOW;
				std::lock_guard<std::mutex> lk(dev->m_report_mutex[deviceNr]);
				
				memcpy(&dev->m_report[deviceNr], report, sizeof(GLOVE_REPORT));

				dev->UpdateState();
				dev->m_report_cv[deviceNr].notify_all();
			}
		}
	}

	hid_close(dev->m_device);

}

void Device::UpdateState() {

	for (int devNr = 0; devNr < DEVICE_TYPE_COUNT; devNr++) {

		if (m_report[devNr].device_id) {
			m_report[devNr].device_id =(device_type_t) 0; // re-using as data-freshness flag



			m_data[devNr].PacketNumber++;

			m_data[devNr].Acceleration.x = m_report[devNr].accel[0] / ACCEL_DIVISOR;
			m_data[devNr].Acceleration.y = m_report[devNr].accel[1] / ACCEL_DIVISOR;
			m_data[devNr].Acceleration.z = m_report[devNr].accel[2] / ACCEL_DIVISOR;

			// normalize quaternion data
			m_data[devNr].Quaternion.w = m_report[devNr].quat[0] / QUAT_DIVISOR;
			m_data[devNr].Quaternion.x = m_report[devNr].quat[1] / QUAT_DIVISOR;
			m_data[devNr].Quaternion.y = m_report[devNr].quat[2] / QUAT_DIVISOR;
			m_data[devNr].Quaternion.z = m_report[devNr].quat[3] / QUAT_DIVISOR;

			// normalize finger data
			for (int j = 0; j < GLOVE_FINGERS; j++) {
				// account for finger order
				if (devNr == DEV_GLOVE_RIGHT - DEVICE_TYPE_LOW)
					m_data[devNr].Fingers[j] = m_report[devNr].fingers[j] / FINGER_DIVISOR;
				else
					m_data[devNr].Fingers[j] = m_report[devNr].fingers[GLOVE_FINGERS - (j + 1)] / FINGER_DIVISOR;
			}

			// calculate the euler angles
			ManusMath::GetEuler(&m_data[devNr].Euler, &m_data[devNr].Quaternion);
		}
	}
}




/*
void Device::SetVibration(float power)
{
	GLOVE_RUMBLER_REPORT output;

	// clipping
	if (power < 0.0) power = 0.0;
	if (power > 1.0) power = 1.0;

	output.rumbler = uint16_t(power * std::numeric_limits<uint16_t>::max());

	unsigned char report[sizeof(GLOVE_REPORT) + 1];
	report[0] = 0x00;
	memcpy(report + 1, &output, sizeof(GLOVE_REPORT));

	hid_write(m_device, report, sizeof(report));
}
*/