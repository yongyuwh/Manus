// TestApp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "hidapi.h"
#include <stdint.h>
#include <stdbool.h>
#include <conio.h>
#include <math.h>
#include <Windows.h>
#include <time.h>



#define NORDIC_USB_VENDOR_ID	   0x1915
#define NORDIC_USB_PRODUCT_ID      0x007B


void print_hex_memory_(void *mem, int size) {
	int i;
	unsigned char *p = (unsigned char *)mem;
	// this is slow!!!! damn
	for (i = 0; i<size; i++) {
		printf("0x%02x ", p[i]);

	}
	printf("\n");
}

void print_hex_memory(uint8_t *p) {
	// this is much faster
	
	// skip byte 0 as this is the device id
	printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
		p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15], p[16], p[17], p[18], p[19], p[20]);
}

/*
void print_print(int size) {
	fputs("printf(\"",stdout);
	for (int i = 0; i < size; i++) fputs("0x%02x ", stdout);
	putchar('"');
	for (int i = 0; i < size; i++) printf(",p[%u] ", i);
	puts(");");
		
}
*/
int main() {
	struct hid_device_info *hid_devices, *current_device;
	hid_devices = hid_enumerate(NORDIC_USB_VENDOR_ID, NORDIC_USB_PRODUCT_ID);
	/*
	current_device = hid_devices;
	while (current_device != nullptr) {


		// Examine the next HID device
		current_device = current_device->next;
	}
	*/
	if (!hid_devices) exit(EXIT_FAILURE);

	hid_device *dev = hid_open_path(hid_devices->path);
	hid_free_enumeration(hid_devices);
	
	LARGE_INTEGER current[4] = { 0 }, previous[4] = { 0 }, elapsed[4] = { 0 };
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	float min[4] = { 1000 }, max[4] = { 0 }, tot[4] = { 0 };
	int count[4] = { 0 }; uint8_t running_count[4] = { 0 };
	float running_values[4][256];
	memset(running_values, 0, sizeof(running_values));
	bool running_valid[4] = { false };
	float running_avg[4] = { 0 };

	time_t startup = time(NULL);

	while (true) {
		uint8_t buff[32];
		int read = hid_read(dev, buff, sizeof(buff));
		if (!read) break;
		uint8_t nr = buff[0] - 2;
		if (nr > 3) continue;

		previous[nr] = current[nr];
		QueryPerformanceCounter(&current[nr]);
		float interval = NAN;
		float avg = NAN;
		if (previous[nr].QuadPart) {
			elapsed[nr].QuadPart = current[nr].QuadPart - previous[nr].QuadPart;
			interval = (elapsed[nr].QuadPart * 1000) / (double)freq.QuadPart;
			if (interval > max[nr]) max[nr] = interval;
			if (interval < min[nr]) min[nr] = interval;
			tot[nr] += interval;
			avg = tot[nr] / count[nr]++;
			running_values[nr][running_count[nr]++] = interval;
		}
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { (SHORT)0, (SHORT)(6 * nr) });

		
		
		
		for (int i = 0; i < 256; i++) {
			running_avg[nr] += running_values[nr][i];
		}
		if (count[nr] == 255 && running_values[0] != 0) running_valid[nr] = true;
		if (running_valid) running_avg[nr] /= 256; else running_avg[nr] = NAN;
		printf("device # %d     packet count %06u     signal strenght: TODO     program running %u\n", nr, count[nr], time(NULL) - startup);
		printf("interval: %09.3f ms  min: %09.3f ms  max: %09.3f ms  avg: %09.3f ms  running avg: %09.3f ms\n", interval, min[nr], max[nr], avg, running_avg[nr]);
		uint32_t *tx_success_cnt = (uint32_t*)(1+buff) ;
		uint32_t *tx_failure_cnt = (uint32_t*)(9+buff);
		uint32_t *rf_failure_cnt = (uint32_t*)(13+buff) ;
		int32_t *signal_strenght = (int32_t*)(5+buff);
		printf("signal %09d     tx_success_cnt %09u     tx_failure_cnt %09u     rf_failure_cnt %09u      \n", *signal_strenght, *tx_success_cnt, *tx_failure_cnt, *rf_failure_cnt);
		print_hex_memory(buff);
		


	}
    return 0;
}

