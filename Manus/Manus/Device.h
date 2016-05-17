/*
Copyright 2016 Manus VR

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

#define DEVICE_TYPE_START 2
#define DEVICE_TYPE_COUNT 4

enum device_type_t : uint8_t {
	DEV_NONE = 0,
	DEV_GLOVE_LEFT = DEVICE_TYPE_START,
	DEV_GLOVE_RIGHT,
	DEV_BRACELET_LEFT,
	DEV_BRACELET_RIGHT
};

class IDevice
{
protected:
	IDevice() {};

public:
	virtual ~IDevice() {};

	virtual void Connect() = 0;
	virtual void Disconnect() = 0;
	virtual bool IsRunning() const = 0;
	virtual const char* GetDevicePath() const = 0;
	virtual bool GetData(GLOVE_DATA* data, device_type_t device, unsigned int timeout) = 0;
	virtual bool GetFlags(uint8_t &flags, device_type_t device, unsigned int timeout) = 0;
	virtual bool GetRssi(int32_t &rssi, device_type_t device, unsigned int timeout) = 0;
	virtual bool GetBatteryVoltage(uint16_t &voltage, device_type_t device, unsigned int timeout) = 0;
	virtual bool GetBatteryPercentage(uint8_t &percentage, device_type_t device, unsigned int timeout) = 0;

	virtual bool IsConnected(device_type_t device) = 0;

	virtual bool SetVibration(float power, device_type_t dev, unsigned int timeout) = 0;
	virtual bool SetFlags(uint8_t flags, device_type_t device) = 0;
	virtual bool PowerOff(device_type_t device) = 0;
};
