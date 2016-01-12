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

#pragma pack(push, 1) // exact fit - no padding
typedef struct
{
	//uint8_t id;
	uint8_t flags;
	int16_t quat[GLOVE_QUATS]; 
	int16_t accel[GLOVE_AXES];
	uint8_t fingers[GLOVE_FINGERS];
} GLOVE_REPORT;

typedef struct
{
	uint16_t rumbler;
} GLOVE_OUTPUT_REPORT;

typedef struct
{
	int16_t compass[GLOVE_AXES];
} COMPASS_REPORT;
#pragma pack(pop) //back to whatever the previous packing mode was

class Glove
{
private:
	bool m_running;
	uint8_t m_flags;

	GLOVE_DATA m_data;
	unsigned int m_packets;
	GLOVE_REPORT m_report;

	char* m_device_path;
	hid_device* m_device;

	std::thread m_thread;
	std::mutex m_report_mutex;
	std::condition_variable m_report_block;

public:
	Glove(const char* device_path);
	~Glove();

	void Connect();
	void Disconnect();
	bool IsRunning() const { return m_running; }
	const char* GetDevicePath() const { return m_device_path; }
	bool GetData(GLOVE_DATA* data, unsigned int timeout);
	uint8_t GetFlags();
	GLOVE_HAND GetHand();
	void SetVibration(float power);

private:
	static void DeviceThread(Glove* glove);
	void UpdateState();
};
