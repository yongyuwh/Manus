#include "DebugDevice.h"

#include <string.h>
#include <algorithm>
#include <thread>

DebugDevice::DebugDevice()
{
	memset(&m_packetNo, 0, sizeof(m_packetNo));
	memset(&m_flags, 0, sizeof(m_flags));
	memset(&m_poweredOff, false, sizeof(m_poweredOff));
}

DebugDevice::~DebugDevice()
{
}


void DebugDevice::Connect()
{
}

void DebugDevice::Disconnect()
{
}

bool DebugDevice::IsRunning() const
{
	return true;
}

const char* DebugDevice::GetDevicePath() const
{
	return "debug-device";
}

bool DebugDevice::GetData(GLOVE_DATA* data, device_type_t device, unsigned int timeout)
{
	memset(data, 0, sizeof(GLOVE_DATA));

	std::this_thread::sleep_for(std::chrono::milliseconds(std::min(5u, timeout)));

	data->PacketNumber = m_packetNo[device]++;
	for (int i = 0; i < MANUS_FINGERS; i++)
		data->Fingers[i] = float(m_packetNo[device] % 256) / 255.0f;
	data->Quaternion.w = 1.0f; // Identity quaternion
	return true;
}

bool DebugDevice::GetFlags(uint8_t &flags, device_type_t device, unsigned int timeout)
{
	flags = m_flags[device];
	return true;
}

bool DebugDevice::GetRssi(int32_t &rssi, device_type_t device, unsigned int timeout)
{
	rssi = 0;
	return true;
}

bool DebugDevice::GetBatteryVoltage(uint16_t &voltage, device_type_t device, unsigned int timeout)
{
	voltage = 3300;
	return true;
}

bool DebugDevice::GetBatteryPercentage(uint8_t &percentage, device_type_t device, unsigned int timeout)
{
	percentage = 100;
	return true;
}

bool DebugDevice::IsConnected(device_type_t device)
{
	return !m_poweredOff[device];
}

bool DebugDevice::SetVibration(float power, device_type_t dev, unsigned int timeout)
{
	return true;
}

bool DebugDevice::SetFlags(uint8_t flags, device_type_t device)
{
	m_flags[device] = flags;
	return true;
}

bool DebugDevice::PowerOff(device_type_t device)
{
	m_poweredOff[device] = true;
	return true;
}
