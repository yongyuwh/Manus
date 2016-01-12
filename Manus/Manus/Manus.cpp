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
#include "Glove.h"
#include "Devices.h"
#include "SkeletalModel.h"

#ifdef _WIN32
#include "WinDevices.h"
#endif

#include <hidapi.h>
#include <vector>
#include <mutex>


// TODO: Acquire Manus VID/PID
// Using Manus VID/PID as defined in dis.c from GloveCode
#define MANUS_VENDOR_ID         0x0220
#define MANUS_PRODUCT_ID        0x0001
//#define MANUS_GLOVE_PAGE	    0x03
//#define MANUS_GLOVE_USAGE       0x04

bool g_initialized = false;

std::vector<Glove*> g_gloves;
std::mutex g_gloves_mutex;

Devices* g_devices;
SkeletalModel g_skeletal;

int GetGlove(GLOVE_HAND hand, Glove** elem)
{
	std::lock_guard<std::mutex> lock(g_gloves_mutex);

	// cycle through all the gloves until the first one of the desired handedness has been found
	for (int i = 0; i < g_gloves.size(); i++)
	{
		if (g_gloves[i]->GetHand() == hand && g_gloves[i]->IsRunning())
		{
			*elem = g_gloves[i];
			return MANUS_SUCCESS;
		}
	}

	return MANUS_DISCONNECTED;
}

void DeviceConnected(const char* device_path)
{
	std::lock_guard<std::mutex> lock(g_gloves_mutex);

	// Check if the glove already exists
	for (Glove* glove : g_gloves)
	{
		if (strcmp(device_path, glove->GetDevicePath()) == 0)
		{
			// The glove was previously connected, reconnect it
			glove->Connect();
			return;
		}
	}

	struct hid_device_info *hid_device = hid_enumerate_device(device_path);
	
	// The glove hasn't been connected before, add it to the list of gloves	
	g_gloves.push_back(new Glove(device_path));

	hid_free_enumeration(hid_device);
}

int ManusInit()
{
	if (g_initialized)
		return MANUS_ERROR;

	if (hid_init() != 0)
		return MANUS_ERROR;

	if (!g_skeletal.InitializeScene())
		return MANUS_ERROR;

	std::lock_guard<std::mutex> lock(g_gloves_mutex);

	// Enumerate the Manus devices on the system
	struct hid_device_info *hid_devices, *current_device;
	hid_devices = hid_enumerate(MANUS_VENDOR_ID, MANUS_PRODUCT_ID);
	current_device = hid_devices;
	for (int i = 0; current_device != nullptr; ++i)
	{
		g_gloves.push_back(new Glove(current_device->path));
		current_device = current_device->next;
	}
	hid_free_enumeration(hid_devices);

#ifdef _WIN32
	g_devices = new WinDevices();
	g_devices->SetDeviceConnected(DeviceConnected);
#endif

	g_initialized = true;

	return MANUS_SUCCESS;
}

int ManusExit()
{
	if (!g_initialized)
		return MANUS_ERROR;

	std::lock_guard<std::mutex> lock(g_gloves_mutex);

	for (Glove* glove : g_gloves)
		delete glove;
	g_gloves.clear();

#ifdef _WIN32
	delete g_devices;
#endif

	g_initialized = false;

	return MANUS_SUCCESS;
}

int ManusGetData(GLOVE_HAND hand, GLOVE_DATA* data, unsigned int timeout)
{
	// Get the glove from the list
	Glove* elem;
	int ret = GetGlove(hand, &elem);
	if (ret != MANUS_SUCCESS)
		return ret;

	if (!data)
		return MANUS_INVALID_ARGUMENT;

	return elem->GetData(data, timeout) ? MANUS_SUCCESS : MANUS_ERROR;
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
	Glove* elem;
	int ret = GetGlove(hand, &elem);
	
	if (ret != MANUS_SUCCESS)
		return ret;

	elem->SetVibration(power);

	return MANUS_SUCCESS;
}
