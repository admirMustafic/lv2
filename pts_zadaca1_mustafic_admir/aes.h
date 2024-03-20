#ifndef __AES128_H_
#define __AES128_H_

#include "uart.h"


void encDataAES(uint8_t* output, uint8_t* input, uint32_t length);
void encBufferAES(uint8_t* output, uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv);
void decBufferAES(uint8_t* output, uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv);

#endif 
