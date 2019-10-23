#ifndef __TOOLS_H
#define __TOOLS_H
#include "main.h"
#include "stdlib.h"

int32_t		constrain(int32_t value, int32_t min, int32_t max);
uint32_t 	analogRead(ADC_HandleTypeDef *hadc, uint32_t channel);
uint32_t	Random(uint32_t max);
uint32_t	Random(uint32_t min, uint32_t max);

#endif
