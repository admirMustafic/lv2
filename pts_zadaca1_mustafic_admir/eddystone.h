#ifndef __EDDYSTONE_H_
#define __EDDYSTONE_H_

#include <nrf52840.h>
#include <nrf52840_bitfields.h>
#include <stdint.h>
#include "uart.h"
#include "aes.h"

#define EDDYSTONE_BASE_FRAME_SIZE				39
#define EDDYSTONE_FRAME_UUID					0x00
#define EDDYSTONE_FRAME_URL						0x10
#define EDDYSTONE_FRAME_TLM						0x20

#define EDDYSTONE_FRAME_LEN_IDX					15
#define EDDYSTONE_FRAME_TYPE_IDX				19
#define EDDYSTONE_POWER_AT_1M					0xEE
#define EDDYSTONE_BEACON_UUID_LEN				16
#define EDDYSTONE_BEACON_URL_LEN				16

#define EDDYSTONE_FRAME_URL_SCHEME_HTTP			0x02
#define EDDYSTONE_FRAME_URL_SCHEME_HTTPS		0x03
#define EDDYSTONE_FRAME_URL_SCHEME_HTTPWWW		0x00
#define EDDYSTONE_FRAME_URL_SCHEME_HTTPSWWW		0x01

#define EDDYSTONE_ENCRYPTION_DISABLED			0x00
#define EDDYSTONE_ENCRYPTION_ENABLED			0x01

#define RADIO_MAX_PAYLOAD_LEN					37
#define RADIO_MAX_PDU							39

#define EDDYSTONE_DATA_SIZE						17

// Link Layer specification Section 2.1.2, Core 4.1 page 2503 
#define ADV_CHANNEL_ACCESS_ADDRESS			0x8E89BED6

void initEDDYSTONE(void);
void txBeaconEDDYSTONE(uint8_t enc, uint8_t * beacon_data);
void txDataEDDYSTONE(uint8_t channel, uint8_t * frame, int8_t power);


int8_t getFreq4ChRADIO(uint8_t ch);
#endif 
