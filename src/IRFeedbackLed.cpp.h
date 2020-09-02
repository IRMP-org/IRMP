/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * IRFeedbackLed.cpp.h - Arduino extensions for IR feedback LED handling
 *
 * Copyright (c) 2020 Armin Joachimsmeyer
 *
 * This file is part of IRMP https://github.com/ukw100/IRMP.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

#if defined(ARDUINO)
#ifndef IR_FEEDBACK_LED_CPP_H
#define IR_FEEDBACK_LED_CPP_H

#include "IRFeedbackLed.h"

static bool irmp_irsnd_LedFeedbackEnabled;

#if defined(IRMP_IRSND_ALLOW_DYNAMIC_PINS)
// global variable to hold feedback led pin number. Set to 0 to not activating feedback LED by default.
uint_fast8_t irmp_irsnd_LedFeedbackPin = 0;
#  if defined(FEEDBACK_LED_IS_ACTIVE_LOW)
bool irmp_irsnd_LedFeedbackPinIsActiveLow = true; // global variable to hold feedback led polarity.
#  else
bool irmp_irsnd_LedFeedbackPinIsActiveLow = false; // global variable to hold feedback led polarity.
#  endif
#endif

/*
 * Enable/disable echoing the input signal to the built in (or specified) LED.
 * The name is chosen to enable easy migration from other IR libs.
 * Pin 13 is the pin of the built in LED on the first Arduino boards.
 */
void irmp_irsnd_LEDFeedback(bool aEnableBlinkLed)
{
    irmp_irsnd_LedFeedbackEnabled = aEnableBlinkLed;
    if (aEnableBlinkLed)
    {
#if defined(IRMP_IRSND_ALLOW_DYNAMIC_PINS)
        if(irmp_irsnd_LedFeedbackPin != 0) {
            pinMode(irmp_irsnd_LedFeedbackPin, OUTPUT);
            if (irmp_irsnd_LedFeedbackPinIsActiveLow)
            {
                digitalWrite(irmp_irsnd_LedFeedbackPin, HIGH);
            }
            else
            {
                digitalWrite(irmp_irsnd_LedFeedbackPin, LOW);
            }
        }
#else
        pinModeFast(LED_BUILTIN, OUTPUT);
#  if defined(FEEDBACK_LED_IS_ACTIVE_LOW)
        digitalWriteFast(LED_BUILTIN, HIGH);
#  else
        digitalWriteFast(LED_BUILTIN, LOW);
#  endif
#endif
    }
#if defined(ALLOW_DISABLE_FEEDBACK_LED_EXPLICIT)
    else
    {
        /*
         * Disable here
         * normally this code is never used, since disabling is done by setting irmp_led_feedback to false.
         */
#if defined(IRMP_IRSND_ALLOW_DYNAMIC_PINS)
        if(irmp_irsnd_LedFeedbackPin != 0) {
            pinMode(irmp_irsnd_LedFeedbackPin, INPUT);
            digitalWrite(irmp_irsnd_LedFeedbackPin, LOW); // to disable internal pullup
        }
#else
        pinModeFast(LED_BUILTIN, INPUT);
        digitalWriteFast(LED_BUILTIN, LOW); // to disable internal pullup
#endif
    }
#endif
}

/*
 * Internally used from IRMP_ISR() with -oS it is taken as inline function
 */
#if defined(ESP8266)
void ICACHE_RAM_ATTR irmp_irsnd_SetFeedbackLED(bool aSwitchLedOn)
#elif defined(ESP32)
void IRAM_ATTR irmp_irsnd_SetFeedbackLED(bool aSwitchLedOn)
#else
void irmp_irsnd_SetFeedbackLED(bool aSwitchLedOn)
#endif
{
#if defined(IRMP_IRSND_ALLOW_DYNAMIC_PINS)
    if(irmp_irsnd_LedFeedbackPin != 0) {
        if (irmp_irsnd_LedFeedbackPinIsActiveLow)
        {
            digitalWrite(irmp_irsnd_LedFeedbackPin, !aSwitchLedOn);
        }
        else
        {
            digitalWrite(irmp_irsnd_LedFeedbackPin, aSwitchLedOn);
        }
    }
#else
#  if defined(__AVR__) // As far as I know, there is no active-low built in LED for AVR platform boards
    digitalWriteFast(LED_BUILTIN, aSwitchLedOn);
#  else
        // hope this is fast enough on other platforms
#    if defined(FEEDBACK_LED_IS_ACTIVE_LOW)
        // If the built in LED on the board is active LOW
    digitalWrite(LED_BUILTIN, !aSwitchLedOn);
#    else
    digitalWrite(LED_BUILTIN, aSwitchLedOn);
#    endif
#  endif
#endif
}

#endif // IR_FEEDBACK_LED_CPP_H
#endif // defined(ARDUINO)
