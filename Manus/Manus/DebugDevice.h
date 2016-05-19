#pragma once

#include "Device.h"

class DebugDevice :
	public IDevice
{
public:
	DebugDevice();
	virtual ~DebugDevice();

	virtual void Connect();
	virtual void Disconnect();
	virtual bool IsRunning() const;
	virtual const char* GetDevicePath() const;
	virtual bool GetData(GLOVE_DATA* data, device_type_t device, unsigned int timeout);
	virtual bool GetFlags(uint8_t &flags, device_type_t device, unsigned int timeout);
	virtual bool GetRssi(int32_t &rssi, device_type_t device, unsigned int timeout);
	virtual bool GetBatteryVoltage(uint16_t &voltage, device_type_t device, unsigned int timeout);
	virtual bool GetBatteryPercentage(uint8_t &percentage, device_type_t device, unsigned int timeout);

	virtual bool IsConnected(device_type_t device);

	virtual bool SetVibration(float power, device_type_t dev, unsigned int timeout);
	virtual bool SetFlags(uint8_t flags, device_type_t device);
	virtual bool PowerOff(device_type_t device);

private:
	uint32_t m_packetNo[DEVICE_TYPE_COUNT];
	uint8_t m_flags[DEVICE_TYPE_COUNT];
	bool m_poweredOff[DEVICE_TYPE_COUNT];
};

