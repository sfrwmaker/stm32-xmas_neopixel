#include "max7219.h"

enum	max7219_register {
	MAX7219_REG_NOOP = 0, MAX7219_REG_DIGIT0 = 1, MAX7219_REG_DECODEMODE = 9,
    MAX7219_REG_INTENSITY, MAX7219_REG_SCANLIMIT, MAX7219_REG_SHUTDOWN, MAX7219_REG_DISPLAYTEST = 0xF
};

const static uint8_t hex_digit[] = {
	0B01111110, 0B00110000, 0B01101101, 0B01111001, 0B00110011, 0B01011011, 0B01011111, 0B01110000,
	0B01111111, 0B01111011, 0B01110111, 0B00011111, 0B00001101, 0B00111101, 0B01001111, 0B01000111
};

// Symbol table starting from Ascii 40 code
const static uint8_t char_table[] = {
    0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B10000000, 0B00000001, 0B10000000, 0B00000000,	// 40-47
	0B01111110, 0B00110000, 0B01101101, 0B01111001, 0B00110011, 0B01011011, 0B01011111, 0B01110000,	// 48-55 ('0' - '7')
	0B01111111, 0B01111011, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000,
	0B00000000, 0B01110111, 0B00011111, 0B00001101, 0B00111101, 0B01001111, 0B01000111, 0B00000000,	// 64-71 ('@' - 'G')
	0B00110111, 0B00000000, 0B00000000, 0B00000000, 0B00001110, 0B00000000, 0B00000000, 0B00000000, // 72-79 ('H' - 'O')
	0B01100111, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000,	// 80-87 ('P' - 'W')
	0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00001000,	// 88-95 ('X' - '_')
};

void max7219_create(max7219 *max, SPI_HandleTypeDef *Hspi, GPIO_TypeDef *GPIOx, uint16_t CS) {
    max->hspi		= Hspi;
    max->cs_GPIO	= GPIOx;
    max->cs_PIN		= CS;
    max->bright		= 4;
}

static void max7219_writeRegister(max7219 *max, uint8_t reg, uint8_t value) {
	HAL_GPIO_WritePin(max->cs_GPIO, max->cs_PIN, GPIO_PIN_RESET);
	uint8_t buff[2];
	buff[0]	= reg;
	buff[1]	= value;
	HAL_SPI_Transmit (max->hspi, buff, 2, 5000);
	HAL_GPIO_WritePin(max->cs_GPIO, max->cs_PIN, GPIO_PIN_SET);
    HAL_Delay(10);
}

void max7219_activate(max7219* max, uint8_t switch_on) {
    uint8_t state = 0;
    if (switch_on) state = 1;
    max7219_writeRegister(max, MAX7219_REG_SHUTDOWN, state);
}

void max7219_init(max7219 *max) {
    max7219_writeRegister(max, MAX7219_REG_SCANLIMIT,    7);	// show all 8 digits
    max7219_writeRegister(max, MAX7219_REG_DECODEMODE,   0);	// using an led matrix (not digits)
    max7219_writeRegister(max, MAX7219_REG_DISPLAYTEST,  0);    // no display test
    max7219_writeRegister(max, MAX7219_REG_INTENSITY,    max->bright);	// character intensity: range: 0 to 15
    max7219_writeRegister(max, MAX7219_REG_SHUTDOWN,     0);    // in shutdown mode
    HAL_Delay(100);
    max7219_activate(max, 1);
    max7219_clear(max);
}

uint8_t max7219_intensity(max7219 *max, uint8_t intensity) {
    if (intensity < 16) {
        max->bright = intensity;
        max7219_writeRegister(max, MAX7219_REG_INTENSITY, max->bright);
    }
    return max->bright;
}

void max7219_clear(max7219 *max) {
	for (uint8_t dgt = 0; dgt < 8; ++dgt) {
		max7219_writeRegister(max, dgt+MAX7219_REG_DIGIT0, 0);
	}
}

void max7219_setRow(max7219 *max, uint8_t row, uint8_t value) {
	if (row < 8) {
		max7219_writeRegister(max, row+MAX7219_REG_DIGIT0, value);
	}
}

void max7219_setDigit(max7219 *max, uint8_t pos, uint8_t hex, uint8_t dp) {
	if (pos < 8 && hex < 16) {
		uint8_t v	= hex_digit[hex];
		if (dp) v  |= 0B10000000;
		max7219_writeRegister(max, pos+MAX7219_REG_DIGIT0, v);
	}
}

void max7219_setChar(max7219 *max, uint8_t pos, char value, uint8_t dp) {
	if (value < 40 || value > 'z') value = 40;						// Draw space in case of wrong character
	if (value >= 'a') value -= 32;									// Translate small letter to Big letter
	value -= 40;
	if (pos < 8 && value < sizeof(char_table) ) {
		uint8_t v	= char_table[(uint8_t)value];
		if (dp) v  |= 0B10000000;
		max7219_writeRegister(max, pos+MAX7219_REG_DIGIT0, v);
	}
}

