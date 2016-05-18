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

#include "GloveDevice.h"
#include "ManusMath.h"

#include <hidapi.h>
#include <limits>

// Normalization constants 
#define ACCEL_DIVISOR 16384.0f
#define QUAT_DIVISOR 16384.0f
#define COMPASS_DIVISOR 32.0f
#define FINGER_DIVISOR 255.0f


GloveDevice::GloveDevice(const char* device_path)
	: m_running(false) {
	size_t len = strlen(device_path) + 1;
	m_device_path = new char[len];
	memcpy(m_device_path, device_path, len * sizeof(char));

	Connect();
}

GloveDevice::~GloveDevice()
{
	Disconnect();
	delete m_device_path;
}


bool GloveDevice::IsConnected(device_type_t device) {
	return  m_running && m_local_stats[device - DEVICE_TYPE_START].packet_count &&
		(clock() - m_local_stats[device - DEVICE_TYPE_START].last_seen) < CLOCKS_PER_SEC;
}


bool GloveDevice::GetData(GLOVE_DATA* data, device_type_t device, unsigned int timeout) {
	if (!IsConnected(device)) return false;

	uint8_t deviceNr = device - DEVICE_TYPE_START;
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

	return IsConnected(device);
	
}

bool GloveDevice::GetFlags(uint8_t & flags, device_type_t device, unsigned int timeout) {
	if (!IsConnected(device)) return false;
	uint8_t deviceNr = device - DEVICE_TYPE_START;

	// Send request for flags
	m_data_out.device_type = device;
	m_data_out.message_type = MSG_FLAGS_GET;

	std::unique_lock<std::mutex> lk(m_flags_mutex[deviceNr]);

	// Optionally wait until the next package is sent
	if (timeout > 0)
	{
		m_flags_cv[deviceNr].wait_for(lk, std::chrono::milliseconds(timeout));
		if (!m_running)
		{
			lk.unlock();
			return false;
		}
	}

	if (m_flags[device - DEVICE_TYPE_START].device_type) {
		flags = m_flags[device - DEVICE_TYPE_START].flags;
		m_flags[device - DEVICE_TYPE_START].device_type = DEV_NONE;
		return true;
	}
	return false;
	
}

bool GloveDevice::GetRssi(int32_t &rssi, device_type_t device, unsigned int timeout) {
	if (!IsConnected(device)) return false;
	uint8_t deviceNr = device - DEVICE_TYPE_START;
	// Send request for stats
	m_data_out.device_type = device;
	m_data_out.message_type = MSG_STATS_GET;

	std::unique_lock<std::mutex> lk(m_stats_mutex[deviceNr]);
	
	// Optionally wait until the next package is sent
	if (timeout > 0)
	{
		
		m_stats_cv[deviceNr].wait_for(lk, std::chrono::milliseconds(timeout));
		if (!m_running)
		{
			lk.unlock();
			return false;
		}
	}
	
	if (m_remote_stats[device - DEVICE_TYPE_START].device_type) {
		rssi = m_remote_stats[device - DEVICE_TYPE_START].stats.tx_rssi;
		m_remote_stats[device - DEVICE_TYPE_START].device_type = DEV_NONE;
		return true;
	}
	return false;
}


bool GloveDevice::GetBatteryVoltage(uint16_t &voltage, device_type_t device, unsigned int timeout) {
	if (!IsConnected(device)) return false;
	uint8_t deviceNr = device - DEVICE_TYPE_START;

	// Send request for stats
	m_data_out.device_type = device;
	m_data_out.message_type = MSG_STATS_GET;

	std::unique_lock<std::mutex> lk(m_stats_mutex[deviceNr]);

	// Optionally wait until the next package is sent
	if (timeout > 0)
	{

		m_stats_cv[deviceNr].wait_for(lk, std::chrono::milliseconds(timeout));
		if (!m_running)
		{
			lk.unlock();
			return false;
		}
	}

	if (m_remote_stats[device - DEVICE_TYPE_START].device_type) {
		voltage = m_remote_stats[device - DEVICE_TYPE_START].stats.battery_voltage;
		m_remote_stats[device - DEVICE_TYPE_START].device_type = DEV_NONE;
		return true;
	}
	return false;
}


bool GloveDevice::GetBatteryPercentage(uint8_t &percentage, device_type_t device, unsigned int timeout) {
	if (!IsConnected(device)) return false;
	uint8_t deviceNr = device - DEVICE_TYPE_START;

	// Send request for stats
	m_data_out.device_type = device;
	m_data_out.message_type = MSG_STATS_GET;

	std::unique_lock<std::mutex> lk(m_stats_mutex[deviceNr]);

	// Optionally wait until the next package is sent
	if (timeout > 0)
	{

		m_stats_cv[deviceNr].wait_for(lk, std::chrono::milliseconds(timeout));
		if (!m_running)
		{
			lk.unlock();
			return false;
		}
	}

	if (m_remote_stats[device - DEVICE_TYPE_START].device_type) {
		percentage = m_remote_stats[device - DEVICE_TYPE_START].stats.battery_percentage;
		m_remote_stats[device - DEVICE_TYPE_START].device_type = DEV_NONE;
		return true;
	}
	return false;
}


bool GloveDevice::SetVibration(float power, device_type_t device, unsigned int timeout) {
	if (!IsConnected(device)) return false;
	// clipping
	if (power < 0) power = 0.0f;
	if (power > 1) power = 1.0f;

	m_data_out.device_type = device;
	m_data_out.message_type = MSG_RUMBLE_PWR;
	m_data_out.rumble.power = (uint16_t)(0xFFFF * power);
	return true;
}


bool GloveDevice::SetFlags(uint8_t flags, device_type_t device) {
	if (!IsConnected(device)) return false;
	m_data_out.device_type = device;
	m_data_out.message_type = MSG_FLAGS_SET;
	m_data_out.flags.flags = flags;
	return true;
}

bool GloveDevice::PowerOff(device_type_t device) {
	if (!IsConnected(device)) return false;
	m_data_out.device_type = device;
	m_data_out.message_type = MSG_POWER_OFF;
	return true;
}

void GloveDevice::Connect() {
	Disconnect();
	m_thread = std::thread(DeviceThread, this);
}

void GloveDevice::Disconnect() {
	// Instruct the device thread to stop and
	// wait for it to shut down.
	m_running = false;
	m_data_out.device_type = DEV_NONE;
	if (m_thread.joinable())
		m_thread.join();
}

void GloveDevice::DeviceThread(GloveDevice* dev) {
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
			int write = hid_write(dev->m_device, (uint8_t*)(&data), sizeof(data));
			
			dev->m_data_out.device_type = DEV_NONE;
		}

		uint8_t report[32];
		//int read = hid_read(dev->m_device, report, sizeof(report));
		int read = hid_read_timeout(dev->m_device, report, sizeof(report), HID_READ_TIMEOUT_MS);

		if (read == 0) continue;

		if (read == -1) {
			dev->m_running = false;
			break;
		}
			

		{

			if (report[0] == DEVICE_MESSAGE) {
				ESB_DATA_PACKET *recv_data = (ESB_DATA_PACKET *)(1 + report);
				if (recv_data->device_type < DEVICE_TYPE_COUNT + DEVICE_TYPE_START) {
					
					uint8_t deviceNr = recv_data->device_type - DEVICE_TYPE_START;
					switch (recv_data->message_type) {
					case MSG_FLAGS_GET: {
						std::lock_guard<std::mutex> lk(dev->m_flags_mutex[deviceNr]);
						dev->m_flags[deviceNr].device_type = recv_data->device_type;
						dev->m_flags[deviceNr].flags = recv_data->flags.flags;
						dev->m_flags_cv[deviceNr].notify_all();
						break;
						}
					case MSG_STATS_GET: {
						std::lock_guard<std::mutex> lk(dev->m_stats_mutex[deviceNr]);
						dev->m_remote_stats[deviceNr].device_type = recv_data->device_type;
						dev->m_remote_stats[deviceNr].stats = recv_data->stats;
						dev->m_stats_cv[deviceNr].notify_all();
						
						break;
						}
					}
				}
			} else if (report[0] < DEVICE_TYPE_COUNT + DEVICE_TYPE_START) {
				uint8_t deviceNr = report[0] - DEVICE_TYPE_START;
				std::lock_guard<std::mutex> lk(dev->m_report_mutex[deviceNr]);
				dev->m_local_stats[deviceNr].packet_count++;
				dev->m_local_stats[deviceNr].last_seen = clock();
				memcpy(&dev->m_report[deviceNr], report, sizeof(GLOVE_REPORT));

				dev->UpdateState();
				dev->m_report_cv[deviceNr].notify_all();
			}
		}
	}

	hid_close(dev->m_device);
	dev->m_device = NULL;
}


void GloveDevice::UpdateState() {

	for (int devNr = 0; devNr < DEVICE_TYPE_COUNT; devNr++) {

		if (m_report[devNr].device_id) {
			m_report[devNr].device_id =(device_type_t) 0; // re-using as data-freshness flag

			m_data[devNr].PacketNumber = m_local_stats[devNr].packet_count;

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
				if (devNr == DEV_GLOVE_RIGHT - DEVICE_TYPE_START)
					m_data[devNr].Fingers[j] = m_report[devNr].fingers[j] / FINGER_DIVISOR;
				else
					m_data[devNr].Fingers[j] = m_report[devNr].fingers[GLOVE_FINGERS - (j + 1)] / FINGER_DIVISOR;
			}

			// calculate the euler angles
			ManusMath::GetEuler(&m_data[devNr].Euler, &m_data[devNr].Quaternion);
		}
	}
}
