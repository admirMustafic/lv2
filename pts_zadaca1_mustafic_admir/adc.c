#include "adc.h"
#include "uart.h"
int16_t g_chip_temp;

uint32_t getBatADC(void)
{
	uint32_t adc_data;

	NRF_SAADC->CH[0].PSELP = (SAADC_CH_PSELP_PSELP_VDD<<SAADC_CH_PSELP_PSELP_Pos);
	NRF_SAADC->CH[0].CONFIG = (SAADC_CH_CONFIG_BURST_Disabled<<SAADC_CH_CONFIG_BURST_Pos)|
		(SAADC_CH_CONFIG_GAIN_Gain1_6<<SAADC_CH_CONFIG_GAIN_Pos)|
		(SAADC_CH_CONFIG_REFSEL_Internal<<SAADC_CH_CONFIG_REFSEL_Pos)|
		(SAADC_CH_CONFIG_MODE_SE<<SAADC_CH_CONFIG_MODE_Pos);

	NRF_SAADC->RESOLUTION = (SAADC_RESOLUTION_VAL_12bit<<SAADC_RESOLUTION_VAL_Pos);

	NRF_SAADC->RESULT.MAXCNT = 1;
	NRF_SAADC->RESULT.PTR = (uint32_t)&adc_data;

	NRF_SAADC->ENABLE = (SAADC_ENABLE_ENABLE_Enabled<<SAADC_ENABLE_ENABLE_Pos);
	NRF_SAADC->TASKS_START = 1;

	while(NRF_SAADC->EVENTS_STARTED==0)
	{
		// Wait for ADC start
	}

	NRF_SAADC->TASKS_SAMPLE = 1;

	while(NRF_SAADC->EVENTS_END==0)
	{
		// Wait for ADC end
	}

	NRF_SAADC->EVENTS_END = 0;
	NRF_SAADC->TASKS_STOP = 1;
	NRF_SAADC->ENABLE = (SAADC_ENABLE_ENABLE_Disabled<<SAADC_ENABLE_ENABLE_Pos);

	uint32_t voltage = (((adc_data&0xFFF)*6.*600.)/(4095));		//(((adc_data&0xFFF)/(4095))) = 0x001 if adc_data is at max ie 0xFFF

	return voltage;
}

void getTempADC(void)
{ // chip temperature
	NVIC_DisableIRQ(TEMP_IRQn);

	// start temperature measurement
	NRF_TEMP->EVENTS_DATARDY = 0;
	NRF_TEMP->TASKS_START = 1;

	NRF_TEMP->INTENSET = 0x01;

	NVIC_EnableIRQ(TEMP_IRQn);
}

void TEMP_IRQHandler(void)
{
	while(NRF_TEMP->EVENTS_DATARDY==0)
	{
		// wait for temp sensor ADC to convert value
	}
	NRF_TEMP->EVENTS_DATARDY =0;
	g_chip_temp = NRF_TEMP->TEMP;
	NRF_TEMP->TASKS_STOP = 1;
}
