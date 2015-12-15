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

#pragma once

#include "Devices.h"

#include <functional>
#include <thread>

class WinDevices :
	public Devices
{
private:
	bool m_running;
	HANDLE m_thread;
	DWORD m_thread_id;

public:
	WinDevices();
	~WinDevices();

private:
	static DWORD WINAPI WinDevices::DeviceThread(LPVOID param);
	static LRESULT CALLBACK WinProcCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};
