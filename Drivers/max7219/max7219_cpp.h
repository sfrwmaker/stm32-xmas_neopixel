#ifndef __MAX7219_CPP_H
#define __MAX7219_CPP_H
#include "max7219.h"

class MAX7219 {
public:
	MAX7219(SPI_HandleTypeDef *Hspi, GPIO_TypeDef *GPIOx, uint16_t CS) {
		max7219_create(&m, Hspi, GPIOx, CS);
	}
	void	init(void)									{ max7219_init(&m); }
	void	activate(bool on) 							{ max7219_activate(&m, on); }
	uint8_t intensity(uint8_t intensity)				{ return max7219_intensity(&m, intensity); }
	void 	clear(void)									{ max7219_clear(&m); }
	void	setRow(uint8_t row, uint8_t value)			{ max7219_setRow(&m, row, value); }
	void	setDigit(uint8_t pos, uint8_t hex, bool dp) { max7219_setDigit(&m, pos, hex, dp); }
	void	setChar(uint8_t  pos, char  value, bool dp)	{ max7219_setChar(&m, pos, value, dp); }
private:
	max7219	m;
};



#endif
