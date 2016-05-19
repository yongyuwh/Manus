#pragma once
#include <thread>
#include <mutex>
#include <vector>

// Time in seconds between device scans
#define MANUS_DEVICE_SCAN_INTERVAL 10

// We're going to support Bluetooth and USB, they're going to have
// different VID/PID so we must support multiple values here

#define NORDIC_USB_VENDOR_ID	   0x1915
#define NORDIC_USB_PRODUCT_ID      0x007B

typedef struct {
	uint16_t VID;
	uint16_t PID;
} MANUS_ID;

// Cross platform clearing the differences between WIN32 and POSIX APIs
#ifdef _WIN32
#define strcasecmp _strcmpi
#endif

class DeviceManager {
public:
	DeviceManager();
	~DeviceManager();

	void EnableDebugMode();
private:
	std::thread DeviceThread;
	std::condition_variable cv;
	std::mutex cv_m;
	bool Running;
	bool DebugMode;

	void EnumerateDevices();
	void EnumerateDevicesThread();
};
