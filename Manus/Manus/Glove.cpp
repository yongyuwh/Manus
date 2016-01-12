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
#include "Glove.h"
#include "ManusMath.h"

#include <hidapi.h>
#include <limits>

// Normalization constants 
#define ACCEL_DIVISOR 16384.0f
#define QUAT_DIVISOR 16384.0f
#define COMPASS_DIVISOR 32.0f
#define FINGER_DIVISOR 255.0f


Glove::Glove(const char* device_path)
	: m_running(false)
{
	//memset(&m_report, 0, sizeof(m_report));

	size_t len = strlen(device_path) + 1;
	m_device_path = new char[len];
	memcpy(m_device_path, device_path, len * sizeof(char));

	Connect();
}

Glove::~Glove()
{
	Disconnect();
	delete m_device_path;
}

bool Glove::GetData(GLOVE_DATA* data, unsigned int timeout)
{
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

	*data = m_data;

	lk.unlock();

	return m_data.PacketNumber > 0;
}

void Glove::Connect()
{
	Disconnect();
	m_thread = std::thread(DeviceThread, this);
}

void Glove::Disconnect()
{
	// Instruct the device thread to stop and
	// wait for it to shut down.
	m_running = false;
	if (m_thread.joinable())
		m_thread.join();
}

void Glove::DeviceThread(Glove* glove)
{
	// TODO: remove threading? (HIDAPI can work without, just return old report when hid_read returns 0)
	glove->m_device = hid_open_path(glove->m_device_path);

	if (!glove->m_device)
		return;

	glove->m_running = true;

	// Keep retrieving reports while the SDK is running and the device is connected
	while (glove->m_running && glove->m_device)
	{
		unsigned char report[sizeof(GLOVE_REPORT)];
		int read = hid_read(glove->m_device, report, sizeof(report));

		if (read == -1)
			break;

		// Set the new data report and notify all blocked callers
		// TODO: Check if the bytes read matches the report size
		{
			std::lock_guard<std::mutex> lk(glove->m_report_mutex);
			memcpy(&glove->m_report, report, sizeof(GLOVE_REPORT));

			glove->UpdateState();

			glove->m_report_block.notify_all();
		}
	}

	hid_close(glove->m_device);

	glove->m_running = false;
	glove->m_report_block.notify_all();
}

void Glove::UpdateState()
{
	m_data.PacketNumber++;

	m_data.Acceleration.x = m_report.accel[0] / ACCEL_DIVISOR;
	m_data.Acceleration.y = m_report.accel[1] / ACCEL_DIVISOR;
	m_data.Acceleration.z = m_report.accel[2] / ACCEL_DIVISOR;

	// normalize quaternion data
	m_data.Quaternion.w = m_report.quat[0] / QUAT_DIVISOR;
	m_data.Quaternion.x = m_report.quat[1] / QUAT_DIVISOR;
	m_data.Quaternion.y = m_report.quat[2] / QUAT_DIVISOR;
	m_data.Quaternion.z = m_report.quat[3] / QUAT_DIVISOR;

	// normalize finger data
	for (int i = 0; i < GLOVE_FINGERS; i++)
	{
		// account for finger order
		if (GetHand() == GLOVE_RIGHT)
			m_data.Fingers[i] = m_report.fingers[i] / FINGER_DIVISOR;
		else
			m_data.Fingers[i] = m_report.fingers[GLOVE_FINGERS - (i + 1)] / FINGER_DIVISOR;
	}

	// calculate the euler angles
	ManusMath::GetEuler(&m_data.Euler, &m_data.Quaternion);
}

uint8_t Glove::GetFlags()
{
	return m_report.flags;
}

GLOVE_HAND Glove::GetHand() {
	return (m_report.flags & GLOVE_FLAGS_HANDEDNESS) ? GLOVE_RIGHT : GLOVE_LEFT;
}

void Glove::SetVibration(float power)
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
