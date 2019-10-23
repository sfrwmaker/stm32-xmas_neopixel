#ifndef __WS2811B_H
#define __WS2811B_H
#include "main.h"

/*
 * Neopixel strip consists of WS2811B RGB LESs.
 * The color of each led coded by three bytes: G-R-B.
 * These bytes are allocated tight one-by-one. So, first three bytes are G-R-B of the first led, second three bytes - of second led and so on.
 * Data array is allocated dynamically as continuous array of bytes of size N*3, where N is the number of LEDs in neopixel strip.
 * Data is transfered to neopixel strip bit-by-bit in the following order: G7,G6,G5,...G0,R7,R6,...R0,B7,B6,...B0.
 * That is why the data array has the color order G-R-B.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef	uint32_t	COLOR;									// Always WRGB

/*
 *  The neopixel strip type. The 4-half bytes (4 bits) code in the order W-R-G-B
 *  For R-G-B part the pure offset is specified, for W the offset + 1 is specified
 */
enum e_neo_type {
	NEO_RGB		= 0x0012,
	NEO_RBG		= 0x0021,
	NEO_GBR		= 0x0201,
	NEO_GRB		= 0x0102,
	NEO_BRG		= 0x0120,
	NEO_BGR		= 0x0210,

	NEO_WRGB	= 0x1012,
	NEO_WRBG	= 0x1021,
	NEO_WGBR	= 0x1201,
	NEO_WGRB	= 0x1102,
	NEO_WBRG	= 0x1120,
	NEO_WBGR	= 0x1210
};
typedef enum e_neo_type	NEO_TYPE;

#define DMA_BUFF_SIZE	64									// two pixels, 32-bit color (in case of WRGB neopixel)

struct s_WS2811B {
	TIM_HandleTypeDef	*htim;								// Pointer to the timer handler
	uint32_t			tim_channel;						// DMA channel of the timer
	DMA_HandleTypeDef 	*hdma;								// DMA handler
	uint8_t				dma[DMA_BUFF_SIZE];					// DMA buffer to be transferred to PWM timer
	uint8_t 			*data;								// Array of pixel's components [GRB]
	uint16_t			leds;								// The numbed of LEDs in the strip
	uint8_t				pwm_zero, pwm_one;					// Timer period for zero and one
	uint8_t				brightness;							// The LED brightness
	uint8_t				r_offset, g_offset, b_offset;		// The RGB offsets in the output sequence
	uint8_t				w_offset;							// The white offset in the output sequence (if applicable)
	uint8_t				bytes_per_led;						// 3 or 4
	volatile uint16_t 	out_index;							// The index of the current displayed pixel from the data buffer
	volatile uint8_t	ready;								// The flag indicating that no DMA transfer is in progress
};
typedef struct s_WS2811B WS2811B;

void		WS2811B_init(WS2811B *strip, uint16_t size, TIM_HandleTypeDef *tmr_handle, uint32_t timer_dma_channel, DMA_HandleTypeDef *dma_handle, NEO_TYPE type);
COLOR		WS2811B_color(uint8_t red, uint8_t green, uint8_t blue);
COLOR		WS2811B_colorW(uint8_t white, uint8_t red, uint8_t green, uint8_t blue);
COLOR		WS2811B_wheel(uint8_t wheel_pos);
COLOR		WS2811B_lightWheel(uint8_t wheel_pos);
void 		WS2811B_setPixelColorRGB(WS2811B *strip, uint16_t n, uint8_t red, uint8_t green, uint8_t blue);
void 		WS2811B_setPixelColorWRGB(WS2811B *strip, uint16_t n, uint8_t white, uint8_t red, uint8_t green, uint8_t blue);
void 		WS2811B_setPixelColor(WS2811B *strip, uint16_t n, COLOR c);
COLOR 		WS2811B_getPixelColor(WS2811B *strip, uint16_t n);
void		WS2811B_setBrightness(WS2811B *strip, uint8_t brightness);
uint8_t		WS2811B_getBrightness(WS2811B *strip);
void 		WS2811B_show(WS2811B *strip);
void 		WS2811B_clear(WS2811B *strip);
uint16_t	WS2811B_numPixels(WS2811B *strip);
void 		WS2811B_DMA_CallBack(WS2811B *strip);
void		WS2811B_waitTransfer(WS2811B *strip);

#ifdef __cplusplus
}
#endif

#endif
