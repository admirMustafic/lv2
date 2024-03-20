#ifndef __GPIOINT_H_
#define __GPIOINT_H_

#include <nrf52840.h>
#include <nrf52840_bitfields.h>
#include <system_nrf52840.h>
#include <stdint.h>

#define LED0					13
#define LED1					14
#define LED2					15
#define LED3					16
#define LED_ARRAY_SIZE			4

#define KEY1					11
#define KEY2					12
#define KEY3					24
#define KEY4					25

#define GPIOTE_PIN_IDLE			0x00
#define GPIOTE_PIN_OFF_MODE		0x01


typedef enum
{
	IRQ_IDLE = 0,
	IRQ_DETECTED,
	IRQ_ACTIVE,
	IRQ_DEBOUNCE,

} IRQ_states;

void initGPIOTE(void);
void deinitGPIOTE(void);
void chkGPIOTE(void);

extern uint8_t g_led_btn_state;

extern volatile uint8_t g_gpio_pin;



#endif 
