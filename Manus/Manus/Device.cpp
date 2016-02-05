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
	return false;
}

bool Device::GetData(GLOVE_DATA* data, device_type_t device, unsigned int timeout) {
	// Wait until the thread is done writing a packet
	std::unique_lock<std::mutex> lk(m_report_mutex);

	// Optionally wait until the next package is sent
	if (timeout > 0)
	{
		m_report_block.wait_for(lk, std::chrono::milliseconds(timeout));
		if (!m_running)
		{
			lk.unlock();
			return false;
		}
	}

	*data = m_data[device - DEVICE_TYPE_LOW];

	lk.unlock();

	return m_data[device - DEVICE_TYPE_LOW].PacketNumber > 0;
}

void Device::Connect()
{
	Disconnect();
	m_thread = std::thread(DeviceThread, this);
}

void Device::Disconnect()
{
	// Instruct the device thread to stop and
	// wait for it to shut down.
	m_running = false;
	if (m_thread.joinable())
		m_thread.join();
}

void Device::DeviceThread(Device* dev)
{
	// TODO: remove threading? (HIDAPI can work without, just return old report when hid_read returns 0)
	dev->m_device = hid_open_path(dev->m_device_path);

	if (!dev->m_device)
		return;

	dev->m_running = true;

	// Keep retrieving reports while the SDK is running and the device is connected
	while (dev->m_running && dev->m_device)
	{
		unsigned char report[sizeof(GLOVE_REPORT)];
		int read = hid_read(dev->m_device, report, sizeof(report));

		if (read == -1)
			break;

		// Set the new data report and notify all blocked callers
		// TODO: Check if the bytes read matches the report size
		{
			std::lock_guard<std::mutex> lk(dev->m_report_mutex);
			if (report[0] < DEVICE_TYPE_COUNT + DEVICE_TYPE_LOW) {
				int deviceNr = report[0] - DEVICE_TYPE_LOW;
				memcpy(&dev->m_report[deviceNr], report, sizeof(GLOVE_REPORT));
			}


			dev->UpdateState();

			dev->m_report_block.notify_all();
		}
	}

	hid_close(dev->m_device);

	dev->m_running = false;
	dev->m_report_block.notify_all();
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
				if (devNr == DEV_GLOVE_RIGHT)
					m_data[devNr].Fingers[j] = m_report[devNr].fingers[j] / FINGER_DIVISOR;
				else
					m_data[devNr].Fingers[j] = m_report[devNr].fingers[GLOVE_FINGERS - (j + 1)] / FINGER_DIVISOR;
			}

			// calculate the euler angles
			ManusMath::GetEuler(&m_data[devNr].Euler, &m_data[devNr].Quaternion);
		}
	}
}





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
