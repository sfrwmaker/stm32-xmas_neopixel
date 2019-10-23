#include <stdlib.h>
#include "ws2811b.h"

/*
 * Use Timer PWM signal with DMA enabled to send the data to the NEOPIXEL strip.
 * According to the datasheet, the data transfer should be performed at 800 kHz (period is 1.25uS)
 * bit-by-bit in the following order: G7,G6,G5,...G0,R7,R6,...R0,B7,B6,...B0
 * Bit 0 - 0.35uS HIGH, 0.8uS LOW
 * Bit 1 - 0,7uS  HIGH, 0.6uS LOW
 * Reset (end of transition) signal is stay LOW for at least 50 uS (about 40 periods of 1.25us). We use 2 complete pixels period
 * The data sequence start with one pixel filled with zeroes, then bits of all the leds and thre reset sequence as 2 complete pixels with zeroes:
 * _________<24 bits>______G7G6G5...B1B0...<other pixels>...___________________<48 bits>____________________
 *
 * We use ring DMA buffer capable to save TWO LEDs, 48 words (uint16_t because of 16-bit timer) for 48 bits of color.
 * When the DMA interrupt fires for the half buffer transfered. In this time we need to fill-up the first half of the buffer
 * with the color of the third LED.
 * When the DMA interrupt fires for the full buffer transfered. In this time we need to fill-up the second half of the buffer
 * with the color of the fourth LED and so on.
 *
 *  out_index is a DMA output index. It is used to transfer the strip data to the NEOPIXEL hardware.
 *  While the transfer is in progress, this index incremented by callback procedure by strip->bytes_per_pixel.
 *  When the whole sequence has been transferred to the NEOPIXEL, we should send reset code, at least 50 uS of ZERO signal,
 *  So we put zeroes to the DMA channel for several 'pixels'. See the WS2811B_fillDmaBuffer().
 */

// Reset, end sequence size in pixels. This is a ZERO signal should last for at least 50 uS
#define reset_pixels 2

// Forward local functions declarations
static void WS2811B_fillDmaBuffer(WS2811B *strip, uint8_t *dma);
static void WS2811B_initType(WS2811B *strip, NEO_TYPE type);

void WS2811B_init(WS2811B *strip, uint16_t size, TIM_HandleTypeDef *tmr_handle, uint32_t timer_dma_channel, DMA_HandleTypeDef *dma_handle, NEO_TYPE type) {
	WS2811B_initType(strip, type);
	strip->leds				= 0;
	strip->data				= 0;
	strip->brightness		= 0;							// Do not use brightness, use pure color
	strip->pwm_zero			= 24; 							// 0.35uS;
	strip->pwm_one			= 49;							// 0.70uS (59 65)
	strip->htim				= 0;
	strip->tim_channel		= 0;
	strip->hdma				= 0;
	strip->ready			= 1;							// Strip is ready for new data and for DMA transfer
	strip->out_index		= (size + reset_pixels) * strip->bytes_per_led; 	// All the pixels were transferred
	strip->data = malloc(size * strip->bytes_per_led);
	if (strip->data) {
		strip->leds 			= size;
		strip->htim				= tmr_handle;
		strip->tim_channel		= timer_dma_channel;
		strip->hdma				= dma_handle;
	}
	WS2811B_clear(strip);
}

COLOR WS2811B_color(uint8_t red, uint8_t green, uint8_t blue) {
	uint32_t c = red; c <<= 8;
	c |= green;	c <<= 8;
	c |= blue;
	return c;
}

COLOR WS2811B_colorW(uint8_t white, uint8_t red, uint8_t green, uint8_t blue) {
	uint32_t c = white; c <<= 8;
	c |= red; 	c <<= 8;
	c |= green;	c <<= 8;
	c |= blue;
	return c;
}

COLOR WS2811B_wheel(uint8_t wheel_pos) {
	wheel_pos = 255 - wheel_pos;
	if(wheel_pos < 85) {
		return WS2811B_color(255 - wheel_pos * 3, 0, wheel_pos * 3);
	}
	if (wheel_pos < 170) {
		wheel_pos -= 85;
		return WS2811B_color(0, wheel_pos * 3, 255 - wheel_pos * 3);
	}
	wheel_pos -= 170;
	return WS2811B_color(wheel_pos * 3, 255 - wheel_pos * 3, 0);
}

COLOR WS2811B_lightWheel(uint8_t wheel_pos) {
	wheel_pos = 255 - wheel_pos;
	if(wheel_pos < 85) {
		return WS2811B_color(255 - wheel_pos, 255, wheel_pos * 3);
	}
	if (wheel_pos < 170) {
		wheel_pos -= 85;
		return WS2811B_color(255, wheel_pos * 3, 255 - wheel_pos);
	}
	wheel_pos -= 170;
	return WS2811B_color(wheel_pos * 3, 255 - wheel_pos, 255);
}

void WS2811B_setPixelColor(WS2811B *strip, uint16_t n, COLOR c) {
	if (n < strip->leds) {
		uint16_t index = n * strip->bytes_per_led;			// The first index of the pixel in the data buffer
		while (strip->out_index <= index + strip->bytes_per_led);	// Wait the current pixel transferred to the NEOPIXEL strip

		uint8_t tmp	= c & 0xFF;								// blue
		if (strip->brightness)
			tmp = (tmp * strip->brightness + 128) >> 8;		// '+128' to round the value
		strip->data[index + strip->b_offset]	= tmp;
		c >>= 8;
		tmp	= c & 0xFF;										// green
		if (strip->brightness)
			tmp = (tmp * strip->brightness + 128) >> 8;
		strip->data[index + strip->g_offset]	= tmp;
		c >>= 8;
		tmp	= c & 0xFF;										// red
		if (strip->brightness)
			tmp = (tmp * strip->brightness + 128) >> 8;
		strip->data[index + strip->r_offset]	= tmp;
		if (strip->bytes_per_led > 3) {
			c >>= 8;
			tmp	= c & 0xFF;									// white
			if (strip->brightness)
				tmp = (tmp * strip->brightness + 128) >> 8;
			strip->data[index + strip->w_offset]	= tmp;
		}
	}
}

void WS2811B_setPixelColorWRGB(WS2811B *strip, uint16_t n, uint8_t white, uint8_t red, uint8_t green, uint8_t blue) {
	if (n < strip->leds) {
		if (strip->brightness) {
			white	= (white * strip->brightness + 128) >> 8;
			red		= (red 	 * strip->brightness + 128) >> 8;
			green 	= (green * strip->brightness + 128) >> 8;
			blue 	= (blue  * strip->brightness + 128) >> 8;
		}
		uint16_t index = n * strip->bytes_per_led;

		while (strip->out_index <= index + strip->bytes_per_led);	// Wait the current pixel transferred to the NEOPIXEL strip
		strip->data[index + strip->w_offset]	= white;
		strip->data[index + strip->r_offset]	= red;
		strip->data[index + strip->g_offset]	= green;
		strip->data[index + strip->b_offset]	= blue;
	}
}

void WS2811B_setPixelColorRGB(WS2811B *strip, uint16_t n, uint8_t red, uint8_t green, uint8_t blue) {
	WS2811B_setPixelColorWRGB(strip, n, 0, red, green, blue);
}


COLOR WS2811B_getPixelColor(WS2811B *strip, uint16_t n) {
	if (n >= strip->leds)
		return 0;

	uint16_t index = n * strip->bytes_per_led;
	uint32_t c = 0;
	uint8_t tmp = 0;
	if (strip->bytes_per_led > 3) {
		tmp = strip->data[index + strip->w_offset];
		if (strip->brightness)
			tmp = ((tmp << 8) + (strip->brightness >> 1)) / strip->brightness;
	} else {
		tmp = 0;
	}
	c  	= tmp & 0xFF; c <<= 8;
	tmp = strip->data[index + strip->r_offset];
	if (strip->brightness)
		tmp = ((tmp << 8) + (strip->brightness >> 1)) / strip->brightness;
	c  |= tmp & 0xFF; c <<= 8;
	tmp = strip->data[index + strip->g_offset];
	if (strip->brightness)
		tmp = ((tmp << 8) + (strip->brightness >> 1)) / strip->brightness;
	c  |= tmp & 0xFF; c <<= 8;
	tmp = strip->data[index + strip->b_offset];
	if (strip->brightness)
		tmp = ((tmp << 8) + (strip->brightness >> 1)) / strip->brightness;
	c  |= tmp & 0xFF;
	return c;
}

void WS2811B_setBrightness(WS2811B *strip, uint8_t brightness) {
	if (brightness == strip->brightness)
		return;
	WS2811B_waitTransfer(strip);
	for (uint16_t i = 0; i < strip->leds * strip->bytes_per_led; ++i) {
		uint32_t c = strip->data[i];
		if (c) {
			if (strip->brightness)
				c *= strip->brightness;
			if (brightness) {
				c += brightness >> 1;
				c /= brightness;
			}
			strip->data[i] = c & 0xFF;
		}
	}
	strip->brightness = brightness;
}

uint8_t	WS2811B_getBrightness(WS2811B *strip) {
	return strip->brightness;
}

uint16_t WS2811B_numPixels(WS2811B *strip) {
	return strip->leds;
}

void WS2811B_clear(WS2811B *strip) {
	WS2811B_waitTransfer(strip);
	for (uint16_t i = 0; i < strip->leds*strip->bytes_per_led; ++i)
		strip->data[i] = 0;
}

// Required to be registered as half buffer complete callback procedure
static void nullCB(DMA_HandleTypeDef *_hdma) { }

void WS2811B_show(WS2811B *strip) {
	if (!strip->data)
		return;

	WS2811B_waitTransfer(strip);							// Wait the previous DMA transfer

	strip->ready	 = 0;									// The DMA transfer is in progress. This flag will be cleared in DMA callback
	strip->out_index = 0;
	uint8_t half_buff = strip->bytes_per_led << 3;			// Fill-up half of the DMA buffer with zeros to start the sequence. Actually, one pixel size is bytes_per_pixel * 8;
	for (uint8_t i = 0; i < half_buff; strip->dma[i++] = 0);
	WS2811B_fillDmaBuffer(strip, &strip->dma[half_buff]);	// Fill up the second half of the DMA buffer with the first LED data

	// To enable HALF buffer callback, we need register own handler, even empty one
	HAL_DMA_RegisterCallback(strip->hdma, HAL_DMA_XFER_HALFCPLT_CB_ID, nullCB);
	// Start the DMA transfer to PWM timer; The buffer size is bytes_per_led * 8 * 2
	HAL_TIM_PWM_Start_DMA(strip->htim, strip->tim_channel, (uint32_t*)strip->dma, strip->bytes_per_led << 4);
}

void WS2811B_waitTransfer(WS2811B *strip) {
	while (strip->ready == 0);
}

// This function uses source of HAL_DMA_IRQHandler() built-in function
void WS2811B_DMA_CallBack(WS2811B *strip) {
	DMA_HandleTypeDef *hdma = strip->hdma;
	  uint32_t flag_it = hdma->DmaBaseAddress->ISR;
	  uint32_t source_it = hdma->Instance->CCR;

	  if (((flag_it & (DMA_FLAG_HT1 << hdma->ChannelIndex)) != RESET) && ((source_it & DMA_IT_HT) != RESET)) {
		  // Half Transfer Complete Interrupt management ******************************

		  // Clear the half transfer complete flag
		  __HAL_DMA_CLEAR_FLAG(hdma, __HAL_DMA_GET_HT_FLAG_INDEX(hdma));

		  // First half of the DMA buffer has been transferred, Fill up the new data
		  WS2811B_fillDmaBuffer(strip, strip->dma);

	  } else if (((flag_it & (DMA_FLAG_TC1 << hdma->ChannelIndex)) != RESET) && ((source_it & DMA_IT_TC) != RESET)) {
		  // Transfer empty reset_pixels led buffers after the sequence
		  if (strip->out_index >= (strip->leds + reset_pixels) * strip->bytes_per_led) {
			  // Disable the transfer complete and error interrupt
			  __HAL_DMA_DISABLE_IT(hdma, DMA_IT_TE | DMA_IT_TC);
			  // Change the DMA state
			  hdma->State = HAL_DMA_STATE_READY;
			  HAL_TIM_PWM_Stop_DMA(strip->htim, strip->tim_channel);
			  strip->ready = 1;
		  }

		  // Clear the transfer complete flag
	      __HAL_DMA_CLEAR_FLAG(hdma, __HAL_DMA_GET_TC_FLAG_INDEX(hdma));
	      // Process Unlocked
	      __HAL_UNLOCK(hdma);

	      // Second half of the DMA buffer has been transferred, Fill up the new data
	      WS2811B_fillDmaBuffer(strip, &strip->dma[strip->bytes_per_led << 3]);

	      // The XferCpltCallback changed to . Even if we register own callback!
	      if (hdma->XferCpltCallback != NULL) {
	    	  hdma->XferCpltCallback(hdma);
	      }

	  } else if ((RESET != (flag_it & (DMA_FLAG_TE1 << hdma->ChannelIndex))) && (RESET != (source_it & DMA_IT_TE))) {
		  // Transfer Error Interrupt management **************************************
		  // When a DMA transfer error occurs a hardware clear of its EN bits is performed, disable ALL DMA IT
		  __HAL_DMA_DISABLE_IT(hdma, (DMA_IT_TC | DMA_IT_HT | DMA_IT_TE));
		  // Clear all flags
		  hdma->DmaBaseAddress->IFCR = (DMA_ISR_GIF1 << hdma->ChannelIndex);
		  //Update error code
		  hdma->ErrorCode = HAL_DMA_ERROR_TE;
		  // Change the DMA state
		  hdma->State = HAL_DMA_STATE_READY;
		  // Process Unlocked
		  __HAL_UNLOCK(hdma);

		  if (hdma->XferErrorCallback != NULL) {
			  hdma->XferErrorCallback(hdma);
		  }
	  }
}

// Fill the DMA buffer with PWM values depending on RGB value of the LED. Use out_index to load required data
static void WS2811B_fillDmaBuffer(WS2811B *strip, uint8_t *dma) {
	uint16_t index = strip->out_index;
	if (index < strip->leds * strip->bytes_per_led) {
		uint8_t bit = 0;
		for (uint8_t color = 0; color < strip->bytes_per_led; ++color) {
			uint8_t c = strip->data[index++];
			for (uint8_t i = 0; i < 8; ++i) {
				if (c & 0x80) {
					dma[bit++]	= strip->pwm_one;
				} else {
					dma[bit++]	= strip->pwm_zero;
				}
				c <<= 1;
			}
		}
		strip->out_index = index;							// Shift the index to the next pixel
	} else {												// End of strip means send reset sequence
		for (uint8_t i = 0; i < (strip->bytes_per_led << 3); ++i) {
			dma[i] = 0;
		}
		strip->out_index += strip->bytes_per_led;
	}
}

static void WS2811B_initType(WS2811B *strip, NEO_TYPE type) {
	strip->bytes_per_led	= 3;
	uint32_t type_code = type;
	strip->b_offset			= type_code & 0x3;	type_code >>= 4;
	strip->g_offset			= type_code & 0x3;	type_code >>= 4;
	strip->r_offset			= type_code & 0x3;	type_code >>= 4;
	if (type_code) {
		strip->w_offset			= (type_code & 0x3) - 1;
		strip->bytes_per_led	= 4;
	}
}
