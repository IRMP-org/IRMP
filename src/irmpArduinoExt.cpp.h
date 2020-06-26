/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * irmpArduinoExt.cpp.h - Arduino extensions to the original irmp.c
 *
 * Copyright (c) 2019-2020 Armin Joachimsmeyer
 *
 * This file is part of IRMP https://github.com/ukw100/IRMP.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

//  // Must be included after declaration of irmp_start_bit_detected etc.
#if defined(ARDUINO)
#undef IRSND_H              // We are in IRMP now! Remove old symbol maybe set from former including irsnd.c.h.
#include "IRTimer.cpp.h"    // include code for timer
static bool irmp_led_feedback;

#if (IRMP_ENABLE_PIN_CHANGE_INTERRUPT == 1)
#include "irmpPinChangeInterrupt.cpp.h"
#endif // (IRMP_ENABLE_PIN_CHANGE_INTERRUPT == 1)

#if defined(ALLOW_DYNAMIC_PINS)
uint_fast8_t irmp_input_pin; // global variable to hold input pin number. Is referenced by defining IRMP_INPUT_PIN as irmp_input_pin.

void irmp_init(uint_fast8_t aIrmpInputPin)
{
    irmp_input_pin = aIrmpInputPin;
    irmp_init();
}
#endif // if defined(ALLOW_DYNAMIC_PINS)

void irmp_init(void)
{
#  ifdef IRMP_INPUT_PIN
    pinModeFast(IRMP_INPUT_PIN, INPUT);                                 // set pin to input
#  else
    IRMP_PORT &= ~_BV(IRMP_BIT);                                        // deactivate pullup
    IRMP_DDR &= ~_BV(IRMP_BIT);                                         // set pin to input
#  endif
#  if defined IRMP_ENABLE_PIN_CHANGE_INTERRUPT && (IRMP_ENABLE_PIN_CHANGE_INTERRUPT != 0)
    initPCIInterrupt();
#  else
    initIRTimerForReceive();
#  endif
#  ifdef IRMP_MEASURE_TIMING
    pinModeFast(IRMP_TIMING_TEST_PIN, OUTPUT);
#  endif
}

/*
 * Echoes the input signal to the built in LED.
 * The name is chosen to enable easy migration from other IR libs.
 * Pin 13 is the pin of the built in LED on the first Arduino boards.
 */
void irmp_LEDFeedback(bool aEnableBlinkLed)
{
    irmp_led_feedback = aEnableBlinkLed;
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

#if defined(__AVR__)
void irmp_debug_print(const __FlashStringHelper * aMessage, bool aDoShortOutput)
#else
void irmp_debug_print(const char * aMessage, bool aDoShortOutput)
#endif
{
    Serial.print(aMessage);
    Serial.print(' ');
    Serial.print(irmp_ir_detected); // valid IR command detected
    Serial.print(F(" St"));
    Serial.print(irmp_start_bit_detected);

    Serial.print(F(" Ws"));
    Serial.print(wait_for_space); // true if in data/address section and no signal. Now increment pause time.
    Serial.print(F(" Wss"));
    Serial.print(wait_for_start_space); // true if we have received start bit

    Serial.print(F(" L"));
    Serial.print(irmp_param.complete_len); // maximum bit position
    Serial.print(F(" B"));
    Serial.print((int8_t) irmp_bit); // current bit position - FF(-1) is start value
    Serial.print(F(" Pu"));
    Serial.print(irmp_pulse_time); // bit time for pulse
    Serial.print(F(" Pa"));
    Serial.print(irmp_pause_time);

    Serial.print(F(" Sb=0x"));
    Serial.print(irmp_param.stop_bit, HEX);

    if (!aDoShortOutput)
    {
        Serial.print(F(" F"));
        Serial.print(irmp_flags); // currently only repetition flag
        Serial.print(F(" K"));
        Serial.print(key_repetition_len); // the pause after a command to distinguish repetitions from new commands
        Serial.print(F(" R"));
        Serial.print(repetition_frame_number); // Number of repetitions
    }

    Serial.println();
}

/*
 * Print protocol name or number, address, code and repetition flag
 * needs appr. 2 milliseconds for output
 */
void irmp_result_print(Print * aSerial, IRMP_DATA * aIRMPDataPtr)
{
    /*
     * Print protocol name or number
     */
    aSerial->print(F("P="));
#if IRMP_PROTOCOL_NAMES == 1
#  if defined(__AVR__)
    uint8_t tProtocolNumber = aIRMPDataPtr->protocol;
    for (uint8_t i = 0; i < sizeof(irmp_used_protocol_index); ++i) {
        if(pgm_read_byte(&irmp_used_protocol_index[i]) == tProtocolNumber) {
            const char* tProtocolStringPtr = (char*) pgm_read_word(&irmp_used_protocol_names[i]);
            aSerial->print((__FlashStringHelper *) (tProtocolStringPtr));
            break;
        }
    }
#  else
    // no need to save space
    aSerial->print(irmp_protocol_names[aIRMPDataPtr->protocol]);
#  endif
    aSerial->print(' ');
#else
    aSerial->print(F("0x"));
    aSerial->print(aIRMPDataPtr->protocol, HEX);
#endif

    /*
     * Print address, code and repetition flag
     */
    aSerial->print(F(" A=0x"));
    aSerial->print(aIRMPDataPtr->address, HEX);
    aSerial->print(F(" C=0x"));
    aSerial->print(aIRMPDataPtr->command, HEX);
    if (aIRMPDataPtr->flags & IRMP_FLAG_REPETITION)
    {
        aSerial->print(F(" R"));
    }
    aSerial->println();
}

/*
 * Do not just call irmp_result_print( &Serial, aIRMPDataPtr), since this is not always possible for ATtinies.
 */
void irmp_result_print(IRMP_DATA * aIRMPDataPtr)
{
    /*
     * Print protocol name or number
     */
    Serial.print(F("P="));
#if IRMP_PROTOCOL_NAMES == 1
#  if defined(__AVR__)
    uint8_t tProtocolNumber = aIRMPDataPtr->protocol;
    for (uint8_t i = 0; i < sizeof(irmp_used_protocol_index); ++i) {
        if(pgm_read_byte(&irmp_used_protocol_index[i]) == tProtocolNumber) {
            const char* tProtocolStringPtr = (char*) pgm_read_word(&irmp_used_protocol_names[i]);
            Serial.print((__FlashStringHelper *) (tProtocolStringPtr));
            break;
        }
    }
#  else
    Serial.print(irmp_protocol_names[aIRMPDataPtr->protocol]);
#  endif
    Serial.print(' ');
#else
    Serial.print(F("0x"));
    Serial.print(aIRMPDataPtr->protocol, HEX);
#endif

    /*
     * Print address, code and repetition flag
     */
    Serial.print(F(" A=0x"));
    Serial.print(aIRMPDataPtr->address, HEX);
    Serial.print(F(" C=0x"));
    Serial.print(aIRMPDataPtr->command, HEX);
    if (aIRMPDataPtr->flags & IRMP_FLAG_REPETITION)
    {
        Serial.print(F(" R"));
    }
    Serial.println();
}
#endif // defined(ARDUINO)
