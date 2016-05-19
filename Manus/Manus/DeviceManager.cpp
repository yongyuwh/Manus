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

#include "Device.h"
#include "DebugDevice.h"
#include "GloveDevice.h"
#include "DeviceManager.h"

#include <thread>
#include <mutex>
#include <string.h>

//extern std::vector<Glove*> g_gloves;
extern std::vector<IDevice*> g_devices;
extern std::mutex g_gloves_mutex;
MANUS_ID ManusId = { NORDIC_USB_VENDOR_ID, NORDIC_USB_PRODUCT_ID };

DeviceManager::DeviceManager()
	: Running(true)
	, DebugMode(false)
{
	DeviceThread = std::thread(&DeviceManager::EnumerateDevicesThread, this);
}

DeviceManager::~DeviceManager()
{
	this->Running = false;
	cv.notify_all();
	DeviceThread.join();
}

/*
Just copying the Manus Emurate code out...
*/
void DeviceManager::EnumerateDevices() {
	std::lock_guard<std::mutex> lock(g_gloves_mutex);

	// Enumerate the Manus devices on the system
	struct hid_device_info *hid_devices, *current_device;
	hid_devices = hid_enumerate(ManusId.VID, ManusId.PID);
	current_device = hid_devices;
	while (current_device != nullptr) {
		bool found = false;
		// The HIDAPI will return gloves we're already connected to.
		// Therefore we will compare the device path for the found device
		// with the device paths known.
		// Walk through the previously detected gloves and compare
		// their device paths to the currently found glove.
		for (IDevice* device : g_devices) {
			if (!(strcasecmp(device->GetDevicePath(), current_device->path))) {
				found = true;
				//Reconnect if previously disconnected
				if (!device->IsRunning()) device->Connect();
				break;
			}
		}

		// If the device isn't previously seen, add it.
		if (!found) g_devices.push_back(new GloveDevice(current_device->path));

		// Examine the next HID device
		current_device = current_device->next;
	}
	hid_free_enumeration(hid_devices);
}

/*
A thread that runs the ManusEnumerate() every 10 seconds
*/
void DeviceManager::EnumerateDevicesThread()
{
	std::unique_lock<std::mutex> lock(cv_m);
	while (this->Running)
	{
		this->EnumerateDevices();
		if (!this->Running) return;
		//std::this_thread::sleep_for(std::chrono::seconds(MANUS_DEVICE_SCAN_INTERVAL));
		cv.wait_for(lock,std::chrono::seconds(MANUS_DEVICE_SCAN_INTERVAL));
		if (!this->Running) return;
	}
}

void DeviceManager::EnableDebugMode()
{
	if (!DebugMode)
		g_devices.push_back(new DebugDevice());
	DebugMode = true;
}
