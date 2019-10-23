#ifndef __BUTTON_H
#define __BUTTON_H
#include "main.h"

class BUTTON {
    public:
        BUTTON(GPIO_TypeDef* ButtonPORT, uint8_t ButtonPIN, uint16_t timeout_ms = 3000);
        void        setTimeout(uint16_t to = 3000)  { over_press = to; }
        uint8_t		intButtonStatus(void);
        void        bINTR(void);
    private:
        GPIO_TypeDef*		b_port;					// The button Port number
        uint32_t    		tick_time;              // The time in ms when the button Tick was set
        uint16_t    		b_pin;                  // The pin number connected to the button
        uint16_t    		over_press;				// Maximum time in ms the button can be pressed
        volatile uint8_t    mode;					// The button mode: 0 - not pressed, 1 - pressed, 2 - long pressed
        volatile uint32_t   pt;						// Time in ms when the button was pressed (press time)
        const uint16_t      tick_timeout = 200;		// Period of button tick, while the button is pressed
        const uint16_t      short_press  = 900;		// If the button was pressed less that this timeout, we assume the short button press
        const uint8_t       bounce      = 50;		// Bouncing timeout (ms)
};

#endif
