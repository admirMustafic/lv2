#ifndef __ADC_H_
#define __ADC_H_

#include <stdbool.h>
#include <stdint.h>
#include <nrf52840.h>
#include <nrf52840_bitfields.h>
#include <system_nrf52840.h>
#include "delay.h"

extern int16_t g_chip_temp;

uint32_t getBatADC(void);
void getTempADC(void);

#endif 
