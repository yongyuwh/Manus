/**
 * Copyright (C) 2015 Manus Machina
 *
 * This file is part of the Manus SDK.
 * 
 * Manus SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Manus SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with Manus SDK. If not, see <http://www.gnu.org/licenses/>.
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
#define MANUS_VENDOR_ID         0x0
#define MANUS_PRODUCT_ID        0x0
#define MANUS_GLOVE_PAGE	    0x03
#define MANUS_GLOVE_USAGE       0x04

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
	if (hid_device->usage_page == MANUS_GLOVE_PAGE && hid_device->usage == MANUS_GLOVE_USAGE)
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
		// We're only interested in devices that identify themselves as VR Gloves
		if (current_device->usage_page == MANUS_GLOVE_PAGE && current_device->usage == MANUS_GLOVE_USAGE)
		{
			g_gloves.push_back(new Glove(current_device->path));
		}
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
