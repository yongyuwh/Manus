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
#include "WinDevices.h"

#include <dbt.h>
#include <Hidsdi.h>

WinDevices::WinDevices()
{
	m_thread = CreateThread(NULL, 0, WinDevices::DeviceThread, this, 0, &m_thread_id);
}

WinDevices::~WinDevices()
{
	m_running = false;
	PostThreadMessage(m_thread_id, WM_QUIT, 0, 0);
	WaitForSingleObject(m_thread, INFINITE);
	CloseHandle(m_thread);
}

DWORD WINAPI WinDevices::DeviceThread(LPVOID param)
{
	WinDevices* devices = (WinDevices*)param;
	devices->m_running = true;

	// Register a ManusDevices class
	WNDCLASS wnd_class;
	memset(&wnd_class, 0, sizeof(wnd_class));
	wnd_class.lpfnWndProc = (WNDPROC)WinProcCallback;
	wnd_class.hInstance = GetModuleHandle(0);
	wnd_class.lpszClassName = L"ManusDevices";
	RegisterClass(&wnd_class);

	// Create a message-only window
	HWND hWnd = CreateWindowEx(0, L"ManusDevices", nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, nullptr);
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)devices);

	// Register a device notification broadcast
	HDEVNOTIFY device_notify;
	DEV_BROADCAST_DEVICEINTERFACE notify_filter;
	memset(&notify_filter, 0, sizeof(notify_filter));
	notify_filter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	notify_filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	HidD_GetHidGuid(&notify_filter.dbcc_classguid);
	device_notify = RegisterDeviceNotification(hWnd, &notify_filter, DEVICE_NOTIFY_WINDOW_HANDLE);

	// Get all messages for the window that belongs to this thread.
	// TODO: Message type filtering.
	MSG msg;
	while (devices->m_running && GetMessage(&msg, nullptr, 0, 0) != 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	devices->m_running = false;
	UnregisterDeviceNotification(device_notify);
	DestroyWindow(hWnd);

	return 0;
}

LRESULT CALLBACK WinDevices::WinProcCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WinDevices* devices = (WinDevices*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	if (message == WM_DEVICECHANGE && wParam == DBT_DEVICEARRIVAL) {
		PDEV_BROADCAST_DEVICEINTERFACE broadcast = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;

		// Convert the device string to a lower case ASCII device path
		size_t len = wcslen(broadcast->dbcc_name);
		char* device_path = new char[len + 1];
		for (size_t i = 0; i < len; i++)
			device_path[i] = tolower(wctob(broadcast->dbcc_name[i]));
		device_path[len] = '\0'; // Don't forget the terminator

		if (devices->m_connected)
			devices->m_connected(device_path);
		return TRUE;
	}
	else
		return DefWindowProc(hWnd, message, wParam, lParam);
}
