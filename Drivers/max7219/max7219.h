#ifndef __MAX7219_H
#define __MAX7219_H
#include "main.h"

/*
 * Device driver for single MAX7219 IC
 */

#ifdef __cplusplus
extern "C" {
#endif

struct s_max7219 {
	SPI_HandleTypeDef	*hspi;									// Pointer to the SPI interface structure
	GPIO_TypeDef 		*cs_GPIO;								// Port of CS pin of max7219 IC
	uint16_t			cs_PIN;									// Pin number of CS
    uint8_t     		bright;                         		// Brightness of the display
};

typedef struct s_max7219 max7219;

void	max7219_create(max7219 *max, SPI_HandleTypeDef *Hspi, GPIO_TypeDef *GPIOx, uint16_t CS);
void	max7219_init(max7219 *max);
void	max7219_activate(max7219* max, uint8_t switch_on);
uint8_t max7219_intensity(max7219 *max, uint8_t intensity);
void 	max7219_clear(max7219 *max);
void	max7219_setRow(max7219 *max, uint8_t row, uint8_t value);
void	max7219_setDigit(max7219 *max, uint8_t pos, uint8_t hex, uint8_t dp);
void	max7219_setChar(max7219 *max, uint8_t pos, char value, uint8_t dp);

#ifdef __cplusplus
}
#endif

#endif
