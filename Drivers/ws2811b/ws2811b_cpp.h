#ifndef __WS2811B_CPP_H
#define __WS2811B_CPP_H
#include "ws2811b.h"

class NEOPIXEL {
	public:
		NEOPIXEL(void)										{ }
		void		init(uint16_t size, TIM_HandleTypeDef *tmr_handle, uint32_t timer_dma_channel, DMA_HandleTypeDef *dma_handle, NEO_TYPE type = NEO_GRB) {
			WS2811B_init(&s, size, tmr_handle, timer_dma_channel, dma_handle, type);
		}
		COLOR 		Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t white = 0) {
			return WS2811B_colorW(white, red, green, blue);
		}
		COLOR		wheel(uint8_t wheel_pos) {
			return WS2811B_wheel(wheel_pos);
		}
		COLOR		lightWheel(uint8_t wheel_pos) {
			return WS2811B_lightWheel(wheel_pos);
		}
		void 		setPixelColor(uint16_t n, uint8_t red, uint8_t green, uint8_t blue, uint8_t white = 0) {
			WS2811B_setPixelColorWRGB(&s, n, white, red, green, blue);
		}
		void 		setPixelColor(uint16_t n, COLOR c) {
			WS2811B_setPixelColor(&s, n, c);
		}
		COLOR 		getPixelColor(uint16_t n) {
			return WS2811B_getPixelColor(&s, n);
		}
		void		setBrightness(uint8_t brightness) {
			WS2811B_setBrightness(&s, brightness);
		}
		uint8_t		getBrightness(void) {
			return WS2811B_getBrightness(&s);
		}
		void 		show(void) {
			WS2811B_show(&s);
		}
		void 		clear(void) {
			WS2811B_clear(&s);
		}
		uint16_t	numPixels(void) {
			return WS2811B_numPixels(&s);
		}
		void 		DMA_CallBack(void) {
			WS2811B_DMA_CallBack(&s);
		}
		void		waitTransfer(void) {
			WS2811B_waitTransfer(&s);
		}
	private:
		WS2811B	s;
};

#endif
