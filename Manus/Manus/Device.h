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
#include <vector>
#include <inttypes.h>

// flag for handedness (0 = left, 1 = right)
#define GLOVE_FLAGS_HANDEDNESS  0x1
#define GLOVE_FLAGS_CAL_GYRO    0x2
#define GLOVE_FLAGS_CAL_ACCEL   0x4
#define GLOVE_FLAGS_CAL_FINGERS 0x8


#define DEVICE_MESSAGE 1
// Message types
#define MSG_PAIRING    0x00
#define MSG_RUMBLE     0x10
#define MSG_RUMBLE_TIM 0x11
#define MSG_RUMBLE_PWR 0x12
#define MSG_FLAGS_GET  0x20
#define MSG_FLAGS_SET  0x21
#define MSG_STATS_GET  0x30
#define MSG_POWER_OFF  0x40 // as KNUB doesn't have a power button, debug power down sequence after rumble

#define GLOVE_AXES      3
#define GLOVE_QUATS     4
#define GLOVE_FINGERS   5

#define GLOVE_REPORT_ID     1
#define COMPASS_REPORT_ID   2

#define DEVICE_TYPE_LOW   2
#define DEVICE_TYPE_COUNT 4
enum device_type_t : uint8_t {
	DEV_NONE = 0,
	DEV_GLOVE_LEFT = DEVICE_TYPE_LOW,
	DEV_GLOVE_RIGHT,
	DEV_BRACELET_LEFT,
	DEV_BRACELET_RIGHT
};

#pragma pack(push, 1) // exact fit - no padding
typedef struct {
	device_type_t device_id;
	int16_t quat[GLOVE_QUATS]; 
	int16_t accel[GLOVE_AXES];
	uint8_t fingers[GLOVE_FINGERS];
	uint8_t flags;
	int32_t rssi;
} GLOVE_REPORT;

typedef struct {
	uint16_t rumbler;
} GLOVE_RUMBLER_REPORT;




// ESB PACKET DEFINITIONS
typedef struct {
	uint32_t tx_success;
	uint32_t tx_failure;
	uint16_t tx_fail_since_last_success;
	uint16_t rf_failure;
	int32_t  tx_rssi;
	uint16_t battery_level;
} stats_t;


typedef struct {
	uint8_t         message_type;
	device_type_t   device_type;
	uint32_t        device_id;
	union {
		struct { uint32_t dongle_address; } pairing;
		struct { uint8_t  flags; } flags;
		struct { uint16_t power; uint16_t duration; } rumble;
		stats_t stats;
		uint8_t raw[26];
	};
} ESB_DATA_PACKET;

typedef struct {
	uint8_t report_id;
	ESB_DATA_PACKET data;
} USB_OUT_PACKET;
#pragma pack(pop) //back to whatever the previous packing mode was

typedef struct {
	device_type_t device_type;
	uint8_t       flags;
} GLOVE_FLAGS;

typedef struct {
	device_type_t device_type;
	stats_t stats;
} GLOVE_STATS;

typedef struct {
	uint32_t packet_count = 0;
	clock_t last_seen = 0;
} LOCAL_STATS;

class Device
{
private:
	bool m_running;

	GLOVE_DATA		m_data[DEVICE_TYPE_COUNT];
	GLOVE_REPORT	m_report[DEVICE_TYPE_COUNT];

	GLOVE_STATS		m_remote_stats[DEVICE_TYPE_COUNT];
	GLOVE_FLAGS		m_flags[DEVICE_TYPE_COUNT];
	LOCAL_STATS		m_local_stats[DEVICE_TYPE_COUNT];
	

	char* m_device_path;
	hid_device* m_device;

	std::thread m_thread;

	std::mutex m_report_mutex[DEVICE_TYPE_COUNT];
	std::condition_variable m_report_cv[DEVICE_TYPE_COUNT];


	std::mutex		m_flags_mutex;
	std::mutex		m_stats_mutex;
	std::condition_variable m_flags_cv;
	std::condition_variable m_stats_cv;


	ESB_DATA_PACKET m_data_out = { 0 };



public:
	Device(const char* device_path);
	~Device();

	void Connect();
	void Disconnect();
	bool IsRunning() const { return m_running; }
	const char* GetDevicePath() const { return m_device_path; }
	bool GetData(GLOVE_DATA* data, device_type_t device, unsigned int timeout);
	bool GetFlags(uint8_t &flags, device_type_t device, unsigned int timeout);
	bool GetRssi(int32_t &rssi, device_type_t device, unsigned int timeout);
	bool GetBattery(uint16_t &battery, device_type_t device, unsigned int timeout);

	bool IsConnected(device_type_t device);
	
	void SetVibration(float power, device_type_t dev, unsigned int timeout);
	void SetFlags(uint8_t flags, device_type_t device);
	//void SetVibration(float power);
	void PowerOff(device_type_t device);

private:
	static void DeviceThread(Device* dev);
	void UpdateState();
};
