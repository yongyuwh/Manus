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

#pragma once

#include "Manus.h"

#include <hidapi.h>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <inttypes.h>

// flag for handedness (0 = left, 1 = right)
#define GLOVE_FLAGS_HANDEDNESS  0x1
#define GLOVE_FLAGS_CAL_GYRO    0x2
#define GLOVE_FLAGS_CAL_ACCEL   0x4
#define GLOVE_FLAGS_CAL_FINGERS 0x8

#define GLOVE_AXES      3
#define GLOVE_QUATS     4
#define GLOVE_FINGERS   5

#define GLOVE_REPORT_ID     1
#define COMPASS_REPORT_ID   2

#define DEVICE_TYPE_LOW   2
#define DEVICE_TYPE_COUNT 4
enum device_type_t : uint8_t {
	DEV_GLOVE_LEFT = DEVICE_TYPE_LOW,
	DEV_GLOVE_RIGHT,
	DEV_BRACELET_LEFT,
	DEV_BRACELET_RIGHT
} ;

#pragma pack(push, 1) // exact fit - no padding
typedef struct {
	device_type_t device_id;
	int16_t quat[GLOVE_QUATS]; 
	int16_t accel[GLOVE_AXES];
	uint8_t fingers[GLOVE_FINGERS];
} GLOVE_REPORT;

typedef struct {
	uint16_t rumbler;
} GLOVE_RUMBLER_REPORT;
#pragma pack(pop) //back to whatever the previous packing mode was


class Device
{
private:
	bool m_running;

	GLOVE_DATA   m_data[DEVICE_TYPE_COUNT];
	unsigned int m_packets[DEVICE_TYPE_COUNT];
	GLOVE_REPORT m_report[DEVICE_TYPE_COUNT];

	char* m_device_path;
	hid_device* m_device;

	std::thread m_thread;
	std::mutex m_report_mutex;
	std::condition_variable m_report_block;

public:
	Device(const char* device_path);
	~Device();

	void Connect();
	void Disconnect();
	bool IsRunning() const { return m_running; }
	const char* GetDevicePath() const { return m_device_path; }
	bool GetData(GLOVE_DATA* data, device_type_t device, unsigned int timeout);
	bool HasDevice(device_type_t device);
	
	void SetVibration(float power);

private:
	static void DeviceThread(Device* dev);
	void UpdateState();
};
