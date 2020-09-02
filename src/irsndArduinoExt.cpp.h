/*
 * irsndArduinoExt.cpp.h - Arduino extensions to the original irsnd.c
 *
 *  Copyright (C) 2020  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of IRMP https://github.com/ukw100/IRMP.
 *
 *  IRMP is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#if defined(ARDUINO)

#undef _IRMP_H_                 // We are in IRSND now! Remove old symbol set from former including irmp.c.h if we use receive and send in the same user program.
#include "IRTimer.cpp.h"        // include code for timer
#include "IRFeedbackLed.cpp.h"  // include code for Feedback LED

#if defined(IRMP_IRSND_ALLOW_DYNAMIC_PINS)
uint_fast8_t irsnd_output_pin;
#  if defined (__AVR__)
// For fast toggling we additional require port and mask
volatile uint8_t * irsnd_output_pin_input_port;
uint8_t irsnd_output_pin_mask;
#  endif

/*
 * Initialize, and activate feedback LED function
 */
void irsnd_init(uint_fast8_t aIrsndOutputPin, uint_fast8_t aFeedbackLedPin, bool aIrmpLedFeedbackPinIsActiveLow)
{
    irsnd_output_pin = aIrsndOutputPin;
#  if defined(__AVR__)
    // store port and pin mask for fast toggle on AVR
    irsnd_output_pin_input_port = portInputRegister(digitalPinToPort(aIrsndOutputPin));
    irsnd_output_pin_mask = digitalPinToBitMask(aIrsndOutputPin);
#  endif

    irmp_irsnd_LedFeedbackPin = aFeedbackLedPin;
    irmp_irsnd_LedFeedbackPinIsActiveLow = aIrmpLedFeedbackPinIsActiveLow;

    /*
     * enable feedback LED if (aFeedbackLedPin != 0)
     */
    irmp_irsnd_LEDFeedback(aFeedbackLedPin);

    // Do not call irsnd_init_and_store_timer() here, it is done at irsnd_send_data().
    pinModeFast(IRSND_OUTPUT_PIN, OUTPUT);
#  ifdef IRMP_MEASURE_TIMING
    pinModeFast(IRMP_TIMING_TEST_PIN, OUTPUT);
#  endif
}

/*
 * Initialize, and activate feedback LED function
 */
void irsnd_init(uint_fast8_t aIrsndOutputPin, uint_fast8_t aFeedbackLedPin)
{
    irsnd_init(aIrsndOutputPin, aFeedbackLedPin, irmp_irsnd_LedFeedbackPinIsActiveLow);
}

/*
 * Initialize, but avoid activating feedback LED by using 0 as led pin
 */
void irsnd_init(uint_fast8_t aIrsndOutputPin)
{
    irsnd_init(aIrsndOutputPin, irmp_irsnd_LedFeedbackPin, irmp_irsnd_LedFeedbackPinIsActiveLow);
#  if defined(LED_BUILTIN)
    // set pin if we have one at hand
    irmp_irsnd_LedFeedbackPin = LED_BUILTIN;
#  endif
}

#else // defined(IRMP_IRSND_ALLOW_DYNAMIC_PINS)
/*
 * Init function for defined pins
 */
void irsnd_init(void)
{
    // Do not call irsnd_init_and_store_timer() here, it is done at irsnd_send_data().
    pinModeFast(IRSND_OUTPUT_PIN, OUTPUT);
#  ifdef IRMP_MEASURE_TIMING
    pinModeFast(IRMP_TIMING_TEST_PIN, OUTPUT);
#  endif
}
#endif // defined(IRMP_IRSND_ALLOW_DYNAMIC_PINS)

static void irsnd_set_freq(IRSND_FREQ_TYPE freq __attribute__((unused)))
{
    // not supported by now
}

/*
 * Called from irsnd_ISR to set the IR output
 */
#if defined(ESP8266)
void ICACHE_RAM_ATTR irsnd_on(void)
#elif defined(ESP32)
void IRAM_ATTR irsnd_on(void)
#else
void irsnd_on(void)
#endif
{
    if (!irsnd_is_on)
    {
        if (irmp_irsnd_LedFeedbackEnabled)
        {
#if defined(IRMP_IRSND_ALLOW_DYNAMIC_PINS)
            irmp_irsnd_SetFeedbackLED(true);
#else
#  if defined(FEEDBACK_LED_IS_ACTIVE_LOW)
            // If the built in LED on the board is active LOW
            digitalWriteFast(LED_BUILTIN, LOW);
#  else
            digitalWriteFast(LED_BUILTIN, HIGH);
#  endif
#endif
        }
        // Activate IR-LED
#  if defined(IR_OUTPUT_IS_ACTIVE_LOW)
        digitalWriteFast(IRSND_OUTPUT_PIN, LOW);
#  else
        digitalWriteFast(IRSND_OUTPUT_PIN, HIGH);
#  endif
        irsnd_is_on = TRUE;
    }
}

#if defined(ESP8266)
void ICACHE_RAM_ATTR irsnd_off(void)
#elif defined(ESP32)
void IRAM_ATTR irsnd_off(void)
#else
void irsnd_off(void)
#endif
{
    if (irsnd_is_on)
    {
        // Manage feedback LED

        if (irmp_irsnd_LedFeedbackEnabled)
        {
#if defined(IRMP_IRSND_ALLOW_DYNAMIC_PINS)
            irmp_irsnd_SetFeedbackLED(false);
#else
#  if defined(FEEDBACK_LED_IS_ACTIVE_LOW)
            // If the built in LED on the board is active LOW
            digitalWriteFast(LED_BUILTIN, HIGH);
#  else
            digitalWriteFast(LED_BUILTIN, LOW);
#  endif
#endif
        }
        // Deactivate IR-LED
#if defined(IR_OUTPUT_IS_ACTIVE_LOW)
        digitalWriteFast(IRSND_OUTPUT_PIN, HIGH);
#else
        digitalWriteFast(IRSND_OUTPUT_PIN, LOW);
#endif
        irsnd_is_on = FALSE;
    }
}

#endif // defined(ARDUINO)
