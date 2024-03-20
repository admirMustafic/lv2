#include "eddystone.h"
#include "adc.h"
#include "gpioint.h"

const uint8_t c_eddystone_beacon_uuid[EDDYSTONE_BEACON_UUID_LEN] =
{ 0x00, 0x01, 0x02, 0x03,
	0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B,
	0x0C, 0x0D, 0x0E, 0x0F
};

// total BLE packet lenght can not be larger than 39B
// max packet payload is 31 bytes 
volatile uint8_t g_eddystone_base_frame[EDDYSTONE_BASE_FRAME_SIZE] =
{
	0x42, 0x25,															// [ 2B] - Header -> second byte (6 bits are length: 37B)
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,									// [ 5B] - Advertising Address 													 					

	// header: header length, type and flags							   [ 3B]
	0x02, 0x01, 0x06,
	// service advertised: length, type and UUID						   [ 4B]
	0x03, 0x03, 0xAA, 0xFE,

	0x00, 0x16, 0xAA, 0xFE,												// [ 4B]
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00
};

// B8:E5:26:AB:98:81
void initEDDYSTONE(void)
{
	uint32_t utmp32;

	utmp32 = NRF_FICR->DEVICEADDR[1];
	g_eddystone_base_frame[7] = (uint8_t)(((utmp32>>8)&0x000000FF)|0xC0);// per nRF52 SDK instruction 47 & 46 bits are always '11'
	g_eddystone_base_frame[6] = (uint8_t)(utmp32&0x000000FF);
	utmp32 = NRF_FICR->DEVICEADDR[0];
	g_eddystone_base_frame[5] = (uint8_t)((utmp32>>24)&0x000000FF);
	g_eddystone_base_frame[4] = (uint8_t)((utmp32>>16)&0x000000FF);
	g_eddystone_base_frame[3] = (uint8_t)((utmp32>>8)&0x000000FF);
	g_eddystone_base_frame[2] = (uint8_t)(utmp32&0x000000FF);

}


void txBeaconEDDYSTONE(uint8_t enc, uint8_t* beacon_data)
{
	uint8_t k, frame[EDDYSTONE_BASE_FRAME_SIZE];
	// uint8_t * ptr_data;
	// 
		//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
		// assembly encrypted URL frame and send it
		//------------------------------------------------------------------
	for(k = 0;k<(EDDYSTONE_BASE_FRAME_SIZE);k++)
	{
		frame[k] = g_eddystone_base_frame[k];
	}

	frame[EDDYSTONE_FRAME_LEN_IDX] = 23;
	//frame[EDDYSTONE_FRAME_LEN_IDX] = 23;							// max 23!
	frame[EDDYSTONE_FRAME_TYPE_IDX] = (EDDYSTONE_FRAME_URL);			// frame type
	frame[(EDDYSTONE_FRAME_TYPE_IDX)+1] = (EDDYSTONE_POWER_AT_1M);	// power at 1 m in dB -> 18dB
	frame[(EDDYSTONE_FRAME_TYPE_IDX)+2] = (EDDYSTONE_FRAME_URL_SCHEME_HTTP);

	for(k = 0;k<(EDDYSTONE_DATA_SIZE); k++)
	{
		frame[(EDDYSTONE_FRAME_TYPE_IDX)+3+k] = beacon_data[k];
	}


	//	unused code but it doesnt work if I delete or comment this
	//	NO IDEA WHY!? since it isn't used (doesn't recognize it as an Eddystone Beacon without it)
	if(enc==(EDDYSTONE_ENCRYPTION_ENABLED))
	{
		uint8_t enc_data[EDDYSTONE_BEACON_URL_LEN];

		encDataAES(enc_data, beacon_data, 16);
	}


	txDataEDDYSTONE(37, frame, 4);									// send beacon all three advertising channels
	txDataEDDYSTONE(38, frame, 4);
	txDataEDDYSTONE(39, frame, 4);
}

void txDataEDDYSTONE(uint8_t channel, uint8_t* frame, int8_t power)
{
	//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
	// init HF clock
	//----------------------------------------------------------------
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;									// start 16 MHz crystal oscillator 
	NRF_CLOCK->TASKS_HFCLKSTART = 0x00000001;
	while(NRF_CLOCK->EVENTS_HFCLKSTARTED==0);							// wait for the external oscillator to start up

	//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
	// init RADIO
	//----------------------------------------------------------------
	// set BLE 1Mbps mode
	NRF_RADIO->MODE = 0x00000003;

	// Set BLE Inter Frame Space (T_IFS) interval to 150 us.
	NRF_RADIO->TIFS = 150;

	// Enable data whitening, set the maximum payload length and set the
	// access address size (3 + 1 octets).
	NRF_RADIO->PCNF1 = 0x02030000|(RADIO_MAX_PAYLOAD_LEN);

	// Preset the address to use when receive and transmit packets (logical
	// address 0, which is assembled by base address BASE0 and prefix byte PREFIX0.AP0.
	NRF_RADIO->RXADDRESSES = 0x00000001;
	NRF_RADIO->TXADDRESS = 0x00000000;

	// Set CRC length to 3 bytes and do not include access address in calculation of CRC
	NRF_RADIO->CRCCNF = 0x00000103;
	// set CRC polinomial
	NRF_RADIO->CRCPOLY = 0x100065B;

	// Configure the header size. The nRF51822 has 3 fields before the
	// payload field: S0 (1B), LENGTH(up to 256B) and S1 (0B), they define PDU header.
	NRF_RADIO->PCNF0 = 0x00000108;

	// set tx power and input tx buffer
	NRF_RADIO->TXPOWER = power;
	NRF_RADIO->PACKETPTR = (uint32_t)frame;

	//wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
	// select channel, frequency and send the data
	//----------------------------------------------------------------
	NRF_RADIO->DATAWHITEIV = channel&0x3F;							// set channel
	NRF_RADIO->FREQUENCY = getFreq4ChRADIO(channel);					// set channel frequency
	NRF_RADIO->BASE0 = ((ADV_CHANNEL_ACCESS_ADDRESS)<<8)&0xFFFFFF00;

	NRF_RADIO->PREFIX0 = ((ADV_CHANNEL_ACCESS_ADDRESS)>>24)&0x000000FF;
	NRF_RADIO->CRCINIT = 0x00555555;

	NRF_RADIO->TASKS_TXEN = 0x00000001;

	// enable radio and wait for confirmation
	NRF_RADIO->EVENTS_READY = 0U;
	NRF_RADIO->TASKS_TXEN = 1;
	while(NRF_RADIO->EVENTS_READY==0U);

	// initiate transmission and wait for completion
	NRF_RADIO->EVENTS_END = 0U;
	NRF_RADIO->TASKS_START = 1U;
	while(NRF_RADIO->EVENTS_END==0U);

	// disable radio and wait for confirmation							// you can ommit this and use deinit radio
	NRF_RADIO->EVENTS_DISABLED = 0U;
	NRF_RADIO->TASKS_DISABLE = 1U;
	while(NRF_RADIO->EVENTS_DISABLED==0U);


	////wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
	//// disable RADIO
	////------------------------------------------------------------------
	//NRF_RADIO->SHORTS = 0;
	//NRF_RADIO->EVENTS_DISABLED = 0;
	//NRF_RADIO->TASKS_DISABLE = 1;
	//while (NRF_RADIO->EVENTS_DISABLED == 0);

	//NRF_RADIO->EVENTS_DISABLED = 0;
	//NRF_RADIO->TASKS_RXEN = 0;
	//NRF_RADIO->TASKS_TXEN = 0;

}

int8_t getFreq4ChRADIO(uint8_t ch)
{/// conver channel to actual frequency in MHz which will be added to 2400 MHz

	switch(ch)
	{
		case(37):
		{
			return 2;
		}
		case 38:
		{
			return 26;
		}
		case 39:
		{
			return 80;
		}
		default:
		{
			if(ch>39)
				return -1;
			else if(ch<11)
				return 4+(2*ch);
			else
				return 6+(2*ch);
		}
	}
}


