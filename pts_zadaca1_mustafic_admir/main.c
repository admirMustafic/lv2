#include <nrf52840.h>
#include <nrf52840_bitfields.h>
#include <stdint.h>
#include "rtc.h"
#include "uart.h"
#include "delay.h"
#include "eddystone.h"
#include "adc.h"
#include "gpioint.h"

#define	SLEEP_MODE_TIME				2000	//	x1ms
// #define	SLEEP_MODE_TIME				5000	//	x1ms	used for testing so as to confirm that transmisison doesnt begin early

//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
// BLE Eddystone Beacon with or without AES128 encryptions
// - After Beacon transmission nRF52 goes to sleep mode 
//----------------------------------------------------------------------
void initCLK(void);
void getData4EmitMAIN(uint8_t* data);


const uint8_t c_leds[LED_ARRAY_SIZE] = { LED0, LED1, LED2, LED3 };
uint16_t cnt;

int main(void)
{
	uint8_t beacon_data[17];
	uint8_t k;

	cnt = 0;		//used for emit cnt display

	for(k = 0; k<(LED_ARRAY_SIZE); k++) {		// Configure GIO pin as output with standard drive strength.		NRF_P0->PIN_CNF[c_leds[k]] = (GPIO_PIN_CNF_DIR_Output<<GPIO_PIN_CNF_DIR_Pos)|
		(GPIO_PIN_CNF_DRIVE_S0S1<<GPIO_PIN_CNF_DRIVE_Pos)|
			(GPIO_PIN_CNF_INPUT_Connect<<GPIO_PIN_CNF_INPUT_Pos)|
			(GPIO_PIN_CNF_PULL_Disabled<<GPIO_PIN_CNF_PULL_Pos)|
			(GPIO_PIN_CNF_SENSE_Disabled<<GPIO_PIN_CNF_SENSE_Pos);

		NRF_P0->OUTSET = (1<<c_leds[k]);
	}
	//	set intially to zero, all LEDs and BTNs are OFF
	g_led_btn_state = 0x00;

	initUART0(6, 8, UART0_BAUDRATE_115200);								// init UART 
	initCLK();
	initGPIOTE();
	printUART0("\nwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww\n", 0);
	printUART0("TK705 PTS Eddystone Beacon - Zadaca1\n", 0);
	printUART0("-------------------------------------------------------\n", 0);
	printUART0("-> DID: [0x%x%x]\n", NRF_FICR->DEVICEID[0], NRF_FICR->DEVICEID[1]);
	// printUART0("VBATT: %d\n", (getBatADC()/3000)*100);

	//	prepare the DID and HWD MAC address, since it doesnt change
	uint32_t dev_addr_1 = NRF_FICR->DEVICEID[0];
	uint32_t dev_addr_2 = NRF_FICR->DEVICEID[1];
	k = 0;
	beacon_data[k++] = dev_addr_1>>24;
	beacon_data[k++] = dev_addr_1>>16;
	beacon_data[k++] = dev_addr_1>>8;
	beacon_data[k++] = dev_addr_1;
	beacon_data[k++] = dev_addr_2>>24;
	beacon_data[k++] = dev_addr_2>>16;
	beacon_data[k++] = dev_addr_2>>8;
	beacon_data[k++] = dev_addr_2;

	k = 11;
	uint32_t hwd_mac1 = NRF_FICR->DEVICEADDR[0];
	uint32_t hwd_mac2 = NRF_FICR->DEVICEADDR[1];
	beacon_data[k++] = hwd_mac1&0x00FF;
	beacon_data[k++] = (hwd_mac1>>8)&0x00FF;
	beacon_data[k++] = (hwd_mac1>>16)&0x00FF;
	beacon_data[k++] = (hwd_mac1>>24)&0x00FF;
	beacon_data[k++] = hwd_mac2&0x00FF;
	beacon_data[k++] = (hwd_mac2>>8)&0x00FF;


	printUART0("-> MAC: [");
	for(k = 11; k<17; k++)
	{
		if(k<16)
		{
			printUART0("%xb:", beacon_data[k]);
		}
		else
		{
			printUART0("%xb]\n", beacon_data[k]);
		}
	}
	// deinitUART0(6, 8);

	// init RTC1 with RC LF clock
	initWakeupRTC1(SLEEP_MODE_TIME);
	initEDDYSTONE();


	rtc_finish_flag = (RTC_TIMEOUT_FINISHED);
	while(1)
	{
		initUART0(6, 8, UART0_BAUDRATE_115200);
		chkGPIOTE();
		// printUART0("rtc_time [%d], %d\n", NRF_RTC1->COUNTER,rtc_finish_flag);

		if((rtc_finish_flag==(RTC_TIMEOUT_FINISHED)))		//	this means the counter has counted down the 2000ms
		{//	to prevent from emitting when sometghin else changes


			//	prepare data
			getData4EmitMAIN(beacon_data);
			// tx eddystone beacon on all three advertising channels
			txBeaconEDDYSTONE(EDDYSTONE_ENCRYPTION_DISABLED, beacon_data);

			printUART0("-> SYS: sleeping...\n");
			deinitUART0(6, 8);
			deinitSYSTIM();

			NRF_TIMER0->TASKS_STOP = 1;
			NRF_TIMER1->TASKS_STOP = 1;
			NRF_TIMER2->TASKS_STOP = 1;
			NRF_CLOCK->TASKS_HFCLKSTOP = 1;

			rtc_finish_flag = (RTC_TIMEOUT_STARTED);
			cnt++;
		}

		__SEV();
		__WFE();
		__WFE();
	}

	return 0;
}

void initCLK()
{
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while(NRF_CLOCK->EVENTS_HFCLKSTARTED==0)
	{
		// Wait for the external oscillator to start up.
	}

	NRF_CLOCK->LFCLKSRC = (CLOCK_LFCLKSRC_EXTERNAL_Enabled<<CLOCK_LFCLKSRC_EXTERNAL_Pos)||
		(CLOCK_LFCLKSRC_SRC_Xtal<<CLOCK_LFCLKSRC_SRC_Pos);
	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_LFCLKSTART = 1;
	while(NRF_CLOCK->EVENTS_LFCLKSTARTED==0)
		; // wait for the external oscillator to start up
}


void getData4EmitMAIN(uint8_t* data)
{
	getTempADC();
	delay_us(100);		//	wait for the temp meas to occur

	uint8_t k;
	k = 8;

	data[k++] = g_chip_temp>>2;		//divide by 4
	data[k++] = g_led_btn_state;
	data[k++] = (getBatADC()*100)/3000;

	printUART0("-> EDDYSTONE TX-ing frame [%d]:\t [", cnt);
	printUART0("\e[1;31m");
	for(k = 0;k<8;k++)
	{
		printUART0("%xb", data[k]);
	}

	printUART0("\e[1;32m");
	for(;k<11;k++)
	{
		printUART0("%xb", data[k]);
	}

	printUART0("\e[1;33m");
	for(;k<17;k++)
	{
		printUART0("%xb", data[k]);
	}
	printUART0("\e[0m]\n");
	printUART0("\e[0m]\n");
	printUART0("\e[0m]\n");
	printUART0("\e[0m]\n");

}
