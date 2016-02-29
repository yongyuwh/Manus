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

#define _USE_MATH_DEFINES
#include <math.h>
#include <conio.h>
#include <stdint.h>


int _tmain(int argc, _TCHAR* argv[])
{
	ManusInit();



	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);

	printf("Press 'p' to start reading the gloves\n");
	printf("Press 'c' to start the finger calibration procedure\n");

	char in = _getch();
	// reset the cursor position
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), COORD());
	float min[4] = { 1000, 1000, 1000, 1000 }, max[4] = { 0 }, tot[4] = { 0 };
	int count[4] = { 0 }; uint8_t running_count[4] = { 0 };
	float running_values[4][256] = { 0 };
	bool running_valid[4] = { false };
	if (in == 'c')
	{
		GLOVE_HAND hand;
		printf("Press 'r' for right hand or 'l' for left hand\n");
		in = _getch();
		if (in == 'l')
			hand = GLOVE_LEFT;
		else
			hand = GLOVE_RIGHT;

		ManusCalibrate(hand, false, false, true);

		printf("Move the flex sensors across their whole range and then press any key\n");
		_getch();
		ManusCalibrate(hand, false, false, false);

		printf("Calibration finished, press any key to exit\n");
		_getch();
	}
	else if (in == 'p')
	{

		uint8_t test;
		while (true)
		{
			if (_kbhit()) 
			{
				char key = _getch();
				if (key == 'q') break;

				if (key == '0') ManusSetHandedness((GLOVE_HAND)0, false);  //turn left  into right  fb -> fa
				if (key == '1') ManusSetHandedness((GLOVE_HAND)1, true);   //turn right into left fa -> fb

				if (key == 'k') ManusSetVibration(GLOVE_LEFT, 0);
				if (key == 'l') ManusSetVibration(GLOVE_LEFT, 0.2);
				
				if (key == 'e') ManusSetVibration(GLOVE_RIGHT, 0);
				if (key == 'r') ManusSetVibration(GLOVE_RIGHT, 0.2);

				if (key == 'u') ManusPowerOff(GLOVE_LEFT);

			}
			//for (int i = 1; i < 2; i++) // only right
			//for (int i = 0; i < 1; i++) // only left
			for (int i = 0; i < 2; i++)
			{
				GLOVE_HAND hand = (GLOVE_HAND)i;
				LARGE_INTEGER start, end, elapsed;
				QueryPerformanceCounter(&start);

				GLOVE_DATA data = { 0 };
				GLOVE_SKELETAL skeletal = { 0 };

				int32_t rssi = 0;
				uint8_t flags = 0;
				bool flags_valid = false;
				bool rssi_valid = false;

				SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { (SHORT)0, (SHORT)(9 * i) });

				//if (ManusGetData(hand, &data, 1000) == MANUS_SUCCESS)
				if (ManusGetData(hand, &data, 50) == MANUS_SUCCESS)
				{
					printf("glove: %d - %06d %s\n", i, data.PacketNumber, i > 0 ? "Right" : "Left");
					//ManusGetSkeletal(hand, &skeletal);
				}
				else
				{
					printf("glove: %d not found \n", i);
					continue;
				}

				QueryPerformanceCounter(&end);
				elapsed.QuadPart = end.QuadPart - start.QuadPart;
				
				float interval = (elapsed.QuadPart * 1000) / (double)freq.QuadPart;
				if (interval > max[i]) max[i] = interval;
				if (interval < min[i]) min[i] = interval;
				tot[i] += interval;
				float avg = tot[i] / count[i]++;
				running_values[i][running_count[i]++] = interval;
				float running_avg = 0;
				for (int j = 0; j < 256; j++) {
					running_avg += running_values[i][j];
				}
				if (count[i] == 255 && running_values[0] != 0) running_valid[i] = true;
				if (running_valid[i]) running_avg /= 256; else running_avg = NAN;
				printf("interval: %06.3f ms  min: %06.3f ms  max: %06.3f ms  avg: %06.3f ms  running avg: %06.3f ms\n", interval, min[i], max[i], avg, running_avg);


				printf("accel: x: % 1.5f; y: % 1.5f; z: % 1.5f\n", data.Acceleration.x, data.Acceleration.y, data.Acceleration.z);
			
				printf("quats Data: x: % 1.5f; y: % 1.5f; z: % 1.5f; w: % 1.5f \n", data.Quaternion.x, data.Quaternion.y, data.Quaternion.z, data.Quaternion.w);
				printf("quats Skel: x: % 1.5f; y: % 1.5f; z: % 1.5f; w: % 1.5f \n", skeletal.palm.orientation.x , skeletal.palm.orientation.y, skeletal.palm.orientation.z, skeletal.palm.orientation.w);

				printf("euler: x: % 1.5f; y: % 1.5f; z: % 1.5f\n", data.Euler.x * (180.0 / M_PI), data.Euler.y * (180.0 / M_PI), data.Euler.z * (180.0 / M_PI));

				printf("fingers: %f;%f;%f;%f;%f\n", data.Fingers[0], data.Fingers[1], data.Fingers[2], data.Fingers[3], data.Fingers[4]);

				if (0 == (count[i] % 100)) {
					flags_valid = ManusGetFlags(hand, &flags, 1000) == MANUS_SUCCESS;

					if (flags_valid) {
						SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { (SHORT)0, (SHORT)((9 * i) + 7) });
						printf("Flags: 0x%02x (%u)", flags, count[i]); 
					}
					else {
						SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { (SHORT)15, (SHORT)((9 * i) + 7) });
						printf("no flags  (%u)", count[i]);
					}
				}

				if (50 == (count[i] % 100)) {
					rssi_valid = ManusGetRssi(hand, &rssi, 1000) == MANUS_SUCCESS;
					
					if (rssi_valid) {
						SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { (SHORT)40, (SHORT)((9 * i) + 7) });
						printf("rssi: %d  (%u)", rssi, count[i]);
					}
					else {
						SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { (SHORT)65, (SHORT)((9 * i) + 7) });
						printf("no rssi  (%u)", count[i]);
					}
				}
				
				
				
				
			}
			SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), COORD());
		}
	}

	ManusExit();

	return 0;
}

