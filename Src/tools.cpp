#include "tools.h"

/*
 * Arduino constrain() function: limits the value inside the required interval
 */
int32_t constrain(int32_t value, int32_t min, int32_t max) {
	if (value < min)	return min;
	if (value > max)	return max;
	return value;
}

/*
 * Arduino analogRead() function:
 * blocking readings from ADC on the specified channel
 */
uint32_t analogRead(ADC_HandleTypeDef *hadc, uint32_t channel) {
	ADC_ChannelConfTypeDef sConfig;
	sConfig.Channel = channel;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_7CYCLES_5;//ADC_SAMPLETIME_1CYCLE_5;
	HAL_ADC_ConfigChannel(hadc, &sConfig);
	if (HAL_ADC_Start(hadc) != HAL_OK) {
		return 4096;
	}
	if (HAL_IS_BIT_CLR(hadc->Instance->CR1, ADC_CR1_SCAN) && HAL_IS_BIT_CLR(hadc->Instance->SQR1, ADC_SQR1_L)) {
		while(HAL_IS_BIT_CLR(hadc->Instance->SR, ADC_FLAG_EOC));
		return HAL_ADC_GetValue(hadc);
	} else {
		return 4096;
	}
}


/*
 * Arduino random() function: generates pseudo random number from min to max-1
 */

uint32_t Random(uint32_t max) {
	return rand() % max;
}

uint32_t Random(uint32_t min, uint32_t max) {
	if (min < max) {
		return rand() % (max-min) + min;
	}
	return min;
}
