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

#undef _IRMP_H_             // We are in IRSND now! Remove old symbol set from former including irmp.c.h if we use receive and send in the same user program.
#include "IRTimer.cpp.h"    // include code for timer

static volatile bool irsnd_led_feedback;
#if defined(IRMP_IRSND_ALLOW_DYNAMIC_PINS)
uint_fast8_t irsnd_output_pin;
#  if defined (__AVR__)
// For fast toggling we additional require port and mask
volatile uint8_t * irsnd_output_pin_input_port;
uint8_t irsnd_output_pin_mask;
#  endif
/*
 * Init function for dynamic pins
 */
void irsnd_init(uint_fast8_t aIrsndOutputPin)
{
    irsnd_output_pin = aIrsndOutputPin;
#  if defined(__AVR__)
    // store port and pin mask for fast toggle on AVR
    irsnd_output_pin_input_port = portInputRegister(digitalPinToPort(aIrsndOutputPin));
    irsnd_output_pin_mask = digitalPinToBitMask(aIrsndOutputPin);
#  endif

    // Do not call irsnd_init_and_store_timer() here, it is done at irsnd_send_data().
    pinModeFast(IRSND_OUTPUT_PIN, OUTPUT);
#  ifdef IRMP_MEASURE_TIMING
    pinModeFast(IRMP_TIMING_TEST_PIN, OUTPUT);
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

static void irsnd_on(void)
{
    if (!irsnd_is_on)
    {
#if defined(pinModeFast)
        if (irsnd_led_feedback) {
#  if defined(FEEDBACK_LED_IS_ACTIVE_LOW)
            // If the built in LED on the board is active LOW
            digitalWriteFast(LED_BUILTIN, LOW);
#  else
            digitalWriteFast(LED_BUILTIN, HIGH);
#  endif
        }
        pinModeFast(IRSND_OUTPUT_PIN, OUTPUT); // Pin is set to input in ISR if (! irsnd_is_on)
        // start with LED active
#  if defined(IR_OUTPUT_IS_ACTIVE_LOW)
        digitalWriteFast(IRSND_OUTPUT_PIN, LOW);
#  else
        digitalWriteFast(IRSND_OUTPUT_PIN, HIGH);
#  endif
#else // defined(pinModeFast)
        if (irsnd_led_feedback)
        {
            // hope this is fast enough on other platforms
#  if defined(FEEDBACK_LED_IS_ACTIVE_LOW)
            // If the built in LED on the board is active LOW
            digitalWrite(LED_BUILTIN, LOW);
#  else
            digitalWrite(LED_BUILTIN, HIGH);
#  endif
        }
        pinMode(IRSND_OUTPUT_PIN, OUTPUT); // Pin is set to input in ISR if (! irsnd_is_on)
#  if defined(IR_OUTPUT_IS_ACTIVE_LOW)
        digitalWrite(IRSND_OUTPUT_PIN, LOW);
#  else
        digitalWrite(IRSND_OUTPUT_PIN, HIGH);
#  endif
#endif // defined(pinModeFast)
        irsnd_is_on = TRUE;
    }
}

static void irsnd_off(void)
{
    if (irsnd_is_on)
    {
        // Manage feedback LED
        if (irsnd_led_feedback)
        {
#if defined(FEEDBACK_LED_IS_ACTIVE_LOW)
            // If the built in LED on the board is active LOW
            digitalWriteFast(LED_BUILTIN, HIGH);
#else
            digitalWriteFast(LED_BUILTIN, LOW);
#endif
        }
        // Disable IR-LED
#if defined(IR_OUTPUT_IS_ACTIVE_LOW)
        digitalWriteFast(IRSND_OUTPUT_PIN, HIGH);
#else
        digitalWriteFast(IRSND_OUTPUT_PIN, LOW);
#endif
        irsnd_is_on = FALSE;
    }
}

/*
 * Echoes the input signal to the built in LED.
 * The name is chosen to enable easy migration from other IR libs.
 * Pin 13 is the pin of the built in LED on the first Arduino boards.
 */
void irsnd_LEDFeedback(bool aEnableBlinkLed)
{
    irsnd_led_feedback = aEnableBlinkLed;
    if (aEnableBlinkLed)
    {
        pinModeFast(LED_BUILTIN, OUTPUT);
#if defined(FEEDBACK_LED_IS_ACTIVE_LOW)
        digitalWriteFast(LED_BUILTIN, HIGH);
#else
        digitalWriteFast(LED_BUILTIN, LOW);
#endif
    }
}

#endif // defined(ARDUINO)
