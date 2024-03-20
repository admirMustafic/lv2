#include "gpioint.h"
#include "delay.h"
#include "uart.h"

#define IRQ_DEBOUNCE_PERIOD			50		//	x1ms

volatile uint32_t g_port_state = 0x00000000;
volatile uint8_t g_gpio_pin = (GPIOTE_PIN_IDLE);

uint8_t g_led_btn_state;

uint8_t irq_state;

volatile uint32_t irq_timer;

void initGPIOTE(void)
{
	g_port_state = 0x00000000;

	NRF_P0->PIN_CNF[KEY1] = 0x0000000C;
	if(NRF_P0->IN&(1<<(KEY1)))										// based on the current state select the oposite state as trigger
	{
		NRF_P0->PIN_CNF[KEY1] = 0x0003000C;
		g_port_state |= (1<<(KEY1));
	}
	else
	{
		NRF_P0->PIN_CNF[KEY1] = 0x0002000C;
	}


	NRF_P0->PIN_CNF[KEY2] = 0x0000000C;
	if(NRF_P0->IN&(1<<(KEY2)))										// based on the current state select the oposite state as trigger
	{
		NRF_P0->PIN_CNF[KEY2] = 0x0003000C;
		g_port_state |= (1<<(KEY2));
	}
	else
	{
		NRF_P0->PIN_CNF[KEY2] = 0x0002000C;
	}


	NRF_P0->PIN_CNF[KEY3] = 0x0000000C;
	if(NRF_P0->IN&(1<<(KEY3)))										// based on the current state select the oposite state as trigger
	{
		NRF_P0->PIN_CNF[KEY3] = 0x0003000C;
		g_port_state |= (1<<(KEY3));
	}
	else
	{
		NRF_P0->PIN_CNF[KEY3] = 0x0002000C;
	}

	NRF_P0->PIN_CNF[KEY4] = 0x0000000C;
	if(NRF_P0->IN&(1<<(KEY4)))										// based on the current state select the oposite state as trigger
	{
		NRF_P0->PIN_CNF[KEY4] = 0x0003000C;
		g_port_state |= (1<<(KEY4));
	}
	else
	{
		NRF_P0->PIN_CNF[KEY4] = 0x0002000C;
	}

	irq_state = (IRQ_IDLE);

	NRF_GPIOTE->INTENSET = 0x80000000;									// enable port event 
	NRF_GPIOTE->EVENTS_PORT = 0;
	NVIC_EnableIRQ(GPIOTE_IRQn);										// register GPIOTE ISR
}

void deinitGPIOTE(void)
{
	NVIC_DisableIRQ(GPIOTE_IRQn);
	NRF_GPIOTE->INTENCLR = 0xFFFFFFFF;
	NRF_P0->PIN_CNF[KEY1] = 0x00000002;
	NRF_P0->PIN_CNF[KEY2] = 0x00000002;
}

void GPIOTE_IRQHandler(void)
{/// GPIOTE ISR
	NRF_GPIOTE->EVENTS_PORT = 0;

	uint32_t o_port_state = g_port_state;
	uint32_t n_port_state = NRF_P0->IN&((1<<(KEY1))|(1<<(KEY2))|(1<<(KEY3))|(1<<(KEY4)));
	g_port_state = n_port_state;

	if(n_port_state!=o_port_state)
	{
		n_port_state ^= o_port_state;									// only those with change will be 1

		if(n_port_state&(1<<(KEY1)))
		{
			if((g_port_state&(1<<(KEY1)))==0x00000000)
			{
				g_gpio_pin ^= 0x01;


				g_led_btn_state ^= 0x01;
				if(g_led_btn_state&(0x01))
				{
					NRF_P0->OUTCLR = (1<<LED0);
				}
				else if((g_led_btn_state&(0x01))==0x00)
				{
					NRF_P0->OUTSET = (1<<LED0);
				}
			}
			NRF_P0->PIN_CNF[KEY1] ^= 0x00010000;						// we always have to toggle sensitivity
		}

		if(n_port_state&(1<<(KEY2)))
		{
			if((g_port_state&(1<<(KEY2)))==0x00000000)
			{

				g_led_btn_state ^= 0x02;
				if(g_led_btn_state&(0x02))
				{
					NRF_P0->OUTCLR = (1<<LED1);
				}
				else if((g_led_btn_state&(0x02))==0x00)
				{
					NRF_P0->OUTSET = (1<<LED1);
				}

			}
			NRF_P0->PIN_CNF[KEY2] ^= 0x00010000;						// we always have to toggle sensitivity
		}

		if(n_port_state&(1<<(KEY3)))
		{
			if((g_port_state&(1<<(KEY3)))==0x00000000)
			{

				g_led_btn_state ^= 0x04;
				if(g_led_btn_state&(0x04))
				{
					NRF_P0->OUTCLR = (1<<LED2);
				}
				else if((g_led_btn_state&(0x04))==0x00)
				{
					NRF_P0->OUTSET = (1<<LED2);
				}

			}
			NRF_P0->PIN_CNF[KEY3] ^= 0x00010000;						// we always have to toggle sensitivity
		}

		if(n_port_state&(1<<(KEY4)))
		{
			if((g_port_state&(1<<(KEY4)))==0x00000000)
			{
				g_led_btn_state ^= 0x08;
				if(g_led_btn_state&(0x08))
				{
					NRF_P0->OUTCLR = (1<<LED3);
				}
				else if((g_led_btn_state&(0x08))==0x00)
				{
					NRF_P0->OUTSET = (1<<LED3);
				}

			}
			NRF_P0->PIN_CNF[KEY4] ^= 0x00010000;						// we always have to toggle sensitivity
		}
		irq_state = (IRQ_DETECTED);

	}
}


void chkGPIOTE(void)
{
	switch(irq_state)
	{
		case(IRQ_IDLE):
		{
			break;
		}
		case(IRQ_DETECTED):
		{
			initSYSTIM();
			irq_timer = getSYSTIM();
			irq_state = (IRQ_DEBOUNCE);
		}
		case(IRQ_DEBOUNCE):
		{
			if(chk4TimeoutSYSTIM(irq_timer, IRQ_DEBOUNCE_PERIOD)==(SYSTIM_TIMEOUT))
			{
				if((NRF_P0->IN&(1<<KEY1))==0x00)
				{
					irq_state = (IRQ_ACTIVE);
				}
				else if((NRF_P0->IN&(1<<KEY2))==0x00)
				{
					irq_state = (IRQ_ACTIVE);
				}
				else if((NRF_P0->IN&(1<<KEY3))==0x00)
				{
					irq_state = (IRQ_ACTIVE);
				}
				else if((NRF_P0->IN&(1<<KEY4))==0x00)
				{
					irq_state = (IRQ_ACTIVE);
				}
				else
				{
					g_led_btn_state &= ~(0xF0);
					irq_state = (IRQ_IDLE);
				}
			}
			break;
		}
		case(IRQ_ACTIVE):
		{

			if((g_port_state&(1<<(KEY1)))==0x00000000)
			{
				// irq_state = IRQ_DEBOUNCE;

				//	set key to high while it is pressed
				g_led_btn_state |= (0x10);
			}
			else
			{
				//	set key bit to low if it is released
				g_led_btn_state &= ~(0x10);
				// irq_state = (IRQ_IDLE);
			}

			if((g_port_state&(1<<(KEY2)))==0x00000000)
			{
				// irq_state = IRQ_DEBOUNCE;

				//	set key to high while it is pressed
				g_led_btn_state |= (0x20);
			}
			else
			{
				//	set key bit to low if it is released
				g_led_btn_state &= ~(0x20);
				// irq_state = (IRQ_IDLE);
			}

			if((g_port_state&(1<<(KEY3)))==0x00000000)
			{
				// irq_state = IRQ_DEBOUNCE;

				//	set key to high while it is pressed
				g_led_btn_state |= (0x40);
			}
			else
			{
				//	set key bit to low if it is released
				g_led_btn_state &= ~(0x40);
				// irq_state = (IRQ_IDLE);
			}

			if((g_port_state&(1<<(KEY4)))==0x00000000)
			{
				// irq_state = IRQ_DEBOUNCE;

				//	set key to high while it is pressed
				g_led_btn_state |= (0x80);
			}
			else
			{
				//	set key bit to low if it is released
				g_led_btn_state &= ~(0x80);
				// irq_state = (IRQ_IDLE);
			}

			break;
		}
		default:
		{
			break;
		}
	}
}