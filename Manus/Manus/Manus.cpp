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
#include "Manus.h"
#include "Device.h"
#include "SkeletalModel.h"
#include "DeviceManager.h"
#include "SettingsManager.h"
#include <hidapi.h>
#include <vector>
#include <mutex>

bool g_initialized = false;

std::vector<Device*> g_devices;
std::mutex g_gloves_mutex;

DeviceManager *g_device_manager;
SkeletalModel g_skeletal;

int ManusInit()
{
	if (g_initialized)
		return MANUS_ERROR;

	if (hid_init() != 0)
		return MANUS_ERROR;

	if (!g_skeletal.InitializeScene())
		return MANUS_ERROR;

	g_device_manager = new DeviceManager();
	g_initialized = true;

#ifdef MANUS_IK
	// initialize the settingsmanager
	new SettingsManager();
#endif // MANUS_IK

	return MANUS_SUCCESS;
}

int ManusExit()
{
	if (!g_initialized)
		return MANUS_ERROR;

	std::lock_guard<std::mutex> lock(g_gloves_mutex);

	for (Device* device : g_devices)
		delete device;
	g_devices.clear();

	delete g_device_manager;

	g_initialized = false;

	return MANUS_SUCCESS;
}

int ManusGetData(GLOVE_HAND hand, GLOVE_DATA* data, unsigned int timeout)
{
	if (!g_initialized)
		return MANUS_ERROR;

	device_type_t dev = (hand == GLOVE_LEFT) ? DEV_GLOVE_LEFT : DEV_GLOVE_RIGHT;
	for (Device* device : g_devices) {
		if (device->GetData(data, dev, timeout)) {
			return MANUS_SUCCESS;
		}
	}
	return MANUS_DISCONNECTED;

}

int ManusGetSkeletal(GLOVE_HAND hand, GLOVE_SKELETAL* model, unsigned int timeout)
{
	GLOVE_DATA data;

	int ret = ManusGetData(hand, &data, timeout);
	if (ret != MANUS_SUCCESS)
		return ret;

	if (g_skeletal.Simulate(data, model, hand))
		return MANUS_SUCCESS;
	else
		return MANUS_ERROR;
}

int ManusSetVibration(GLOVE_HAND hand, float power){
	device_type_t dev = (hand == GLOVE_LEFT) ? DEV_GLOVE_LEFT : DEV_GLOVE_RIGHT;
	for (Device* device : g_devices) {
		if (!device->IsConnected(dev)) continue;
		device->SetVibration(power, dev, 200);
		return MANUS_SUCCESS;
	}
	return MANUS_DISCONNECTED;
}

int ManusGetFlags(GLOVE_HAND hand, uint8_t* flags, unsigned int timeout) {
	if (!g_initialized)
		return MANUS_ERROR;

	device_type_t dev = (hand == GLOVE_LEFT) ? DEV_GLOVE_LEFT : DEV_GLOVE_RIGHT;
	for (Device* device : g_devices) {
		if (device->GetFlags(*flags, dev, timeout)) {
			return MANUS_SUCCESS;
		}
	}
	return MANUS_DISCONNECTED;
}

int ManusGetRssi(GLOVE_HAND hand, int32_t* rssi, unsigned int timeout) {
	if (!g_initialized)
		return MANUS_ERROR;

	device_type_t dev = (hand == GLOVE_LEFT) ? DEV_GLOVE_LEFT : DEV_GLOVE_RIGHT;
	for (Device* device : g_devices) {
		if (device->GetRssi(*rssi, dev, timeout)) {
			return MANUS_SUCCESS;
		}
	}
	return MANUS_DISCONNECTED;
}

int ManusGetBatteryVoltage(GLOVE_HAND hand, uint16_t* battery, unsigned int timeout) {
	if (!g_initialized)
		return MANUS_ERROR;

	device_type_t dev = (hand == GLOVE_LEFT) ? DEV_GLOVE_LEFT : DEV_GLOVE_RIGHT;
	for (Device* device : g_devices) {
		if (device->GetBatteryVoltage(*battery, dev, timeout)) {
			return MANUS_SUCCESS;
		}
	}
	return MANUS_DISCONNECTED;
}

int ManusGetBatteryPercentage(GLOVE_HAND hand, uint8_t* battery, unsigned int timeout) {
	if (!g_initialized)
		return MANUS_ERROR;

	device_type_t dev = (hand == GLOVE_LEFT) ? DEV_GLOVE_LEFT : DEV_GLOVE_RIGHT;
	for (Device* device : g_devices) {
		if (device->GetBatteryPercentage(*battery, dev, timeout)) {
			return MANUS_SUCCESS;
		}
	}
	return MANUS_DISCONNECTED;
}

int ManusCalibrate(GLOVE_HAND hand, bool gyro, bool accel, bool fingers)
{
	uint8_t flags;
	device_type_t dev = (hand == GLOVE_LEFT) ? DEV_GLOVE_LEFT : DEV_GLOVE_RIGHT;
	Device* flags_device = NULL;
	// Get the glove from the list
	for (Device* device : g_devices) {
		if (device->GetFlags(flags, dev, 100)) {
			flags_device = device;
			break;
		}
	}
	if (!flags_device) return MANUS_DISCONNECTED;

	if (gyro)
		flags |= GLOVE_FLAGS_CAL_GYRO;
	else
		flags &= ~GLOVE_FLAGS_CAL_GYRO;
	if (accel)
		flags |= GLOVE_FLAGS_CAL_ACCEL;
	else
		flags &= ~GLOVE_FLAGS_CAL_ACCEL;
	if (fingers)
		flags |= GLOVE_FLAGS_CAL_FINGERS;
	else
		flags &= ~GLOVE_FLAGS_CAL_FINGERS;
	
	flags_device->SetFlags(flags, dev); 

	return MANUS_SUCCESS;
}


int ManusSetHandedness(GLOVE_HAND hand, bool right_hand)
{
	// Get the glove from the list
	uint8_t flags;
	device_type_t dev = (hand == GLOVE_LEFT) ? DEV_GLOVE_LEFT : DEV_GLOVE_RIGHT;
	Device* flags_device = NULL;
	for (Device* device : g_devices) {
		if (device->GetFlags(flags, dev, 100)) {
			flags_device = device;
			break;
		}
	}

	if (!flags_device) return MANUS_DISCONNECTED;

	if (right_hand)
		flags |= GLOVE_FLAGS_HANDEDNESS;
	else
		flags &= ~GLOVE_FLAGS_HANDEDNESS;

	flags_device->SetFlags(flags, dev);

	return MANUS_SUCCESS;
}

int ManusPowerOff(GLOVE_HAND hand) {
	device_type_t dev = (hand == GLOVE_LEFT) ? DEV_GLOVE_LEFT : DEV_GLOVE_RIGHT;
	for (Device* device : g_devices) {
		if (!device->IsConnected(dev)) continue;
		device->PowerOff(dev);
		return MANUS_SUCCESS;
	}
	return MANUS_DISCONNECTED;
}

bool ManusIsConnected(GLOVE_HAND hand) {
	device_type_t dev = (hand == GLOVE_LEFT) ? DEV_GLOVE_LEFT : DEV_GLOVE_RIGHT;
	for (Device* device : g_devices) {
		if (device->IsConnected(dev)) {
			return true;
		}
	}
	return false;
}