#include "button.h"

BUTTON::BUTTON(GPIO_TypeDef* ButtonPORT, uint8_t ButtonPIN, uint16_t timeout_ms) {
	b_port		= ButtonPORT;
    b_pin		= ButtonPIN;
    over_press	= timeout_ms;
    pt = tick_time = 0;
}

uint8_t	BUTTON::intButtonStatus(void) {
	if (pt > 0) {									// The button was pressed
		if ((HAL_GetTick() - pt) > short_press) {
			mode = 2;								// Long press
			pt = 0;
		}
	}
	uint8_t m = mode; mode = 0;
	return m;
}

void BUTTON::bINTR(void) {							// Interrupt function, called when the button status changed
	uint32_t now_t = HAL_GetTick();
	bool keyUp = (HAL_GPIO_ReadPin(b_port, b_pin) == GPIO_PIN_SET);
	if (!keyUp) {                                  	// The button has been pressed
		if ((pt == 0) || (now_t - pt > over_press)) pt = now_t;
	} else {										// The button was released
		if (pt > 0) {
			if ((now_t - pt) < short_press)
				mode = 1; 							// short press
			else if ((now_t - pt) < over_press)
				mode = 2;                     		// long press
			else
				mode = 0;							// over press
			pt = 0;
		}
	}
}

