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
#include "DeviceManager.h"
#include <hidapi.h>
#include <vector>
#include <mutex>

bool g_initialized = false;

std::vector<Glove*> g_gloves;
std::mutex g_gloves_mutex;

DeviceManager *g_device_manager;
SkeletalModel g_skeletal;

int GetGlove(GLOVE_HAND hand, Glove** elem)
{
	if (!g_initialized)
		return MANUS_ERROR;

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

	delete g_device_manager;

	g_initialized = false;

	return MANUS_SUCCESS;
}

int ManusGetData(GLOVE_HAND hand, GLOVE_DATA* data, unsigned int timeout)
{
	if (!g_initialized)
		return MANUS_ERROR;

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
