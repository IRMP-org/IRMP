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
#undef _IRSND_H_              // We are in IRMP now! Remove old symbol maybe set from former including irsnd.c.h.
#include "IRTimer.cpp.h"    // include code for timer
static bool irmp_led_feedback;

#if defined(IRMP_ENABLE_PIN_CHANGE_INTERRUPT)
#include "irmpPinChangeInterrupt.cpp.h"
#endif // defined(IRMP_ENABLE_PIN_CHANGE_INTERRUPT)

#if defined(IRMP_IRSND_ALLOW_DYNAMIC_PINS)
uint_fast8_t irmp_input_pin; // global variable to hold input pin number. Is referenced by defining IRMP_INPUT_PIN as irmp_input_pin.

void irmp_init(uint_fast8_t aIrmpInputPin)
{
    irmp_input_pin = aIrmpInputPin;
#  if defined IRMP_ENABLE_PIN_CHANGE_INTERRUPT
    initPCIInterrupt();
#  else
    initIRTimerForReceive();
#  endif
#  ifdef IRMP_MEASURE_TIMING
    pinModeFast(IRMP_TIMING_TEST_PIN, OUTPUT);
#  endif
}
#endif // if defined(IRMP_IRSND_ALLOW_DYNAMIC_PINS)

void irmp_init(void)
{
#  ifdef IRMP_INPUT_PIN
    pinModeFast(IRMP_INPUT_PIN, INPUT);                                 // set pin to input
#  else
    IRMP_PORT &= ~_BV(IRMP_BIT);                                        // deactivate pullup
    IRMP_DDR &= ~_BV(IRMP_BIT);                                         // set pin to input
#  endif
#  if defined IRMP_ENABLE_PIN_CHANGE_INTERRUPT
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
 * irmp_used_protocol_index holds the protocol numbers (from irmpprotocols.h)
 * for the included protocol name entries of the irmp_used_protocol_names array below
 * E.g. irmp_used_protocol_index=2,7 and irmp_used_protocol_names="NEC","RC5".
 *
 * Both arrays together are generally smaller than the complete irmp_protocol_names array
 * allowing them to be used on ATtinies even if program code for access is bigger.
 * Flash size is more than 100 bytes less (for 15 protocols) using these arrays.
 */
const uint8_t irmp_used_protocol_index[] PROGMEM =
{
    IRMP_UNKNOWN_PROTOCOL,
#if IRMP_SUPPORT_SIRCS_PROTOCOL == 1
    IRMP_SIRCS_PROTOCOL,
#endif
#if IRMP_SUPPORT_NEC_PROTOCOL == 1
    IRMP_NEC_PROTOCOL,
#endif
#if IRMP_SUPPORT_SAMSUNG_PROTOCOL == 1
    IRMP_SAMSUNG_PROTOCOL,
#endif
#if IRMP_SUPPORT_MATSUSHITA_PROTOCOL == 1
    IRMP_MATSUSHITA_PROTOCOL,
#endif
#if IRMP_SUPPORT_KASEIKYO_PROTOCOL == 1
    IRMP_KASEIKYO_PROTOCOL,
#endif
#if IRMP_SUPPORT_RECS80_PROTOCOL == 1
    IRMP_RECS80_PROTOCOL,
#endif
#if IRMP_SUPPORT_RC5_PROTOCOL == 1
    IRMP_RC5_PROTOCOL,
#endif
#if IRMP_SUPPORT_DENON_PROTOCOL == 1
    IRMP_DENON_PROTOCOL,
#endif
#if IRMP_SUPPORT_RC6_PROTOCOL == 1
    IRMP_RC6_PROTOCOL,
#endif
#if IRMP_SUPPORT_SAMSUNG_PROTOCOL == 1
    IRMP_SAMSUNG32_PROTOCOL,
#endif
#if IRMP_SUPPORT_NEC_PROTOCOL == 1
    IRMP_APPLE_PROTOCOL,
#endif
#if IRMP_SUPPORT_RECS80EXT_PROTOCOL == 1
    IRMP_RECS80EXT_PROTOCOL,
#endif
#if IRMP_SUPPORT_NUBERT_PROTOCOL == 1
    IRMP_NUBERT_PROTOCOL,
#endif
#if IRMP_SUPPORT_BANG_OLUFSEN_PROTOCOL == 1
    IRMP_BANG_OLUFSEN_PROTOCOL,
#endif
#if IRMP_SUPPORT_GRUNDIG_PROTOCOL == 1
    IRMP_GRUNDIG_PROTOCOL,
#endif
#if IRMP_SUPPORT_NOKIA_PROTOCOL == 1
    IRMP_NOKIA_PROTOCOL,
#endif
#if IRMP_SUPPORT_SIEMENS_PROTOCOL  == 1
    IRMP_SIEMENS_PROTOCOL,
#endif
#if IRMP_SUPPORT_FDC_PROTOCOL == 1
    IRMP_FDC_PROTOCOL,
#endif
#if IRMP_SUPPORT_RCCAR_PROTOCOL == 1
    IRMP_RCCAR_PROTOCOL,
#endif
#if IRMP_SUPPORT_JVC_PROTOCOL == 1
    IRMP_JVC_PROTOCOL,
#endif
#if IRMP_SUPPORT_RC6_PROTOCOL == 1
    IRMP_RC6A_PROTOCOL,
#endif
#if IRMP_SUPPORT_NIKON_PROTOCOL == 1
    IRMP_NIKON_PROTOCOL,
#endif
#if IRMP_SUPPORT_RUWIDO_PROTOCOL == 1
    IRMP_RUWIDO_PROTOCOL,
#endif
#if IRMP_SUPPORT_IR60_PROTOCOL == 1
    IRMP_IR60_PROTOCOL,
#endif
#if IRMP_SUPPORT_KATHREIN_PROTOCOL == 1
    IRMP_KATHREIN_PROTOCOL,
#endif
#if IRMP_SUPPORT_NETBOX_PROTOCOL == 1
    IRMP_NETBOX_PROTOCOL,
#endif
#if IRMP_SUPPORT_NEC16_PROTOCOL == 1
    IRMP_NEC16_PROTOCOL,
#endif
#if IRMP_SUPPORT_NEC42_PROTOCOL == 1
    IRMP_NEC42_PROTOCOL,
#endif
#if IRMP_SUPPORT_LEGO_PROTOCOL == 1
    IRMP_LEGO_PROTOCOL,
#endif
#if IRMP_SUPPORT_THOMSON_PROTOCOL == 1
    IRMP_THOMSON_PROTOCOL,
#endif
#if IRMP_SUPPORT_BOSE_PROTOCOL == 1
    IRMP_BOSE_PROTOCOL,
#endif
#if IRMP_SUPPORT_A1TVBOX_PROTOCOL == 1
    IRMP_A1TVBOX_PROTOCOL,
#endif
#if IRMP_SUPPORT_ORTEK_PROTOCOL == 1
    IRMP_ORTEK_PROTOCOL,
#endif
#if IRMP_SUPPORT_TELEFUNKEN_PROTOCOL == 1
    IRMP_TELEFUNKEN_PROTOCOL,
#endif
#if IRMP_SUPPORT_ROOMBA_PROTOCOL == 1
    IRMP_ROOMBA_PROTOCOL,
#endif
#if IRMP_SUPPORT_RCMM_PROTOCOL  == 1
    IRMP_RCMM32_PROTOCOL,
    IRMP_RCMM24_PROTOCOL,
    IRMP_RCMM12_PROTOCOL,
#endif
#if IRMP_SUPPORT_SPEAKER_PROTOCOL == 1
    IRMP_SPEAKER_PROTOCOL,
#endif
#if IRMP_SUPPORT_LGAIR_PROTOCOL == 1
    IRMP_LGAIR_PROTOCOL,
#endif
#if IRMP_SUPPORT_SAMSUNG48_PROTOCOL == 1
    IRMP_SAMSUNG48_PROTOCOL,
#endif
#if IRMP_SUPPORT_MERLIN_PROTOCOL == 1
    IRMP_MERLIN_PROTOCOL,
#endif
#if IRMP_SUPPORT_PENTAX_PROTOCOL == 1
    IRMP_PENTAX_PROTOCOL,
#endif
#if IRMP_SUPPORT_FAN_PROTOCOL == 1
    IRMP_FAN_PROTOCOL,
#endif
#if IRMP_SUPPORT_S100_PROTOCOL == 1
    IRMP_S100_PROTOCOL,
#endif
#if IRMP_SUPPORT_ACP24_PROTOCOL  == 1
    IRMP_ACP24_PROTOCOL,
#endif
#if IRMP_SUPPORT_TECHNICS_PROTOCOL == 1
    IRMP_TECHNICS_PROTOCOL,
#endif
#if IRMP_SUPPORT_PANASONIC_PROTOCOL == 1
    IRMP_PANASONIC_PROTOCOL,
#endif
#if IRMP_SUPPORT_MITSU_HEAVY_PROTOCOL == 1
    IRMP_MITSU_HEAVY_PROTOCOL,
#endif
#if IRMP_SUPPORT_VINCENT_PROTOCOL == 1
    IRMP_VINCENT_PROTOCOL,
#endif
#if IRMP_SUPPORT_SAMSUNGAH_PROTOCOL == 1
    IRMP_SAMSUNGAH_PROTOCOL,
#endif
#if IRMP_SUPPORT_IRMP16_PROTOCOL == 1
    IRMP_IRMP16_PROTOCOL,
#endif
#if IRMP_SUPPORT_GREE_PROTOCOL == 1
    IRMP_GREE_PROTOCOL,
#endif
#if IRMP_SUPPORT_RCII_PROTOCOL == 1
    IRMP_RCII_PROTOCOL,
#endif
#if IRMP_SUPPORT_METZ_PROTOCOL == 1
    IRMP_METZ_PROTOCOL,
#endif
#if IRMP_SUPPORT_NEC_PROTOCOL == 1
    IRMP_ONKYO_PROTOCOL,
#endif
#if IRMP_SUPPORT_RF_GEN24_PROTOCOL == 1
    RF_GEN24_PROTOCOL,
#endif
#if IRMP_SUPPORT_RF_X10_PROTOCOL == 1
    RF_X10_PROTOCOL
#endif
};

const char * const irmp_used_protocol_names[] PROGMEM =
{
    proto_unknown,
#if IRMP_SUPPORT_SIRCS_PROTOCOL == 1
    proto_sircs,
#endif
#if IRMP_SUPPORT_NEC_PROTOCOL == 1
    proto_nec,
#endif
#if IRMP_SUPPORT_SAMSUNG_PROTOCOL == 1
    proto_samsung,
#endif
#if IRMP_SUPPORT_MATSUSHITA_PROTOCOL == 1
    proto_matsushita,
#endif
#if IRMP_SUPPORT_KASEIKYO_PROTOCOL == 1
    proto_kaseikyo,
#endif
#if IRMP_SUPPORT_RECS80_PROTOCOL == 1
    proto_recs80,
#endif
#if IRMP_SUPPORT_RC5_PROTOCOL == 1
    proto_rc5,
#endif
#if IRMP_SUPPORT_DENON_PROTOCOL == 1
    proto_denon,
#endif
#if IRMP_SUPPORT_RC6_PROTOCOL == 1
    proto_rc6,
#endif
#if IRMP_SUPPORT_SAMSUNG_PROTOCOL == 1
    proto_samsung32,
#endif
#if IRMP_SUPPORT_NEC_PROTOCOL == 1
    proto_apple,
#endif
#if IRMP_SUPPORT_RECS80EXT_PROTOCOL == 1
    proto_recs80ext,
#endif
#if IRMP_SUPPORT_NUBERT_PROTOCOL == 1
    proto_nubert,
#endif
#if IRMP_SUPPORT_BANG_OLUFSEN_PROTOCOL == 1
    proto_bang_olufsen,
#endif
#if IRMP_SUPPORT_GRUNDIG_PROTOCOL == 1
    proto_grundig,
#endif
#if IRMP_SUPPORT_NOKIA_PROTOCOL == 1
    proto_nokia,
#endif
#if IRMP_SUPPORT_SIEMENS_PROTOCOL  == 1
    proto_siemens,
#endif
#if IRMP_SUPPORT_FDC_PROTOCOL == 1
    proto_fdc,
#endif
#if IRMP_SUPPORT_RCCAR_PROTOCOL == 1
    proto_rccar,
#endif
#if IRMP_SUPPORT_JVC_PROTOCOL == 1
    proto_jvc,
#endif
#if IRMP_SUPPORT_RC6_PROTOCOL == 1
    proto_rc6a,
#endif
#if IRMP_SUPPORT_NIKON_PROTOCOL == 1
    proto_nikon,
#endif
#if IRMP_SUPPORT_RUWIDO_PROTOCOL == 1
    proto_ruwido,
#endif
#if IRMP_SUPPORT_IR60_PROTOCOL == 1
    proto_ir60,
#endif
#if IRMP_SUPPORT_KATHREIN_PROTOCOL == 1
    proto_kathrein,
#endif
#if IRMP_SUPPORT_NETBOX_PROTOCOL == 1
    proto_netbox,
#endif
#if IRMP_SUPPORT_NEC16_PROTOCOL == 1
    proto_nec16,
#endif
#if IRMP_SUPPORT_NEC42_PROTOCOL == 1
    proto_nec42,
#endif
#if IRMP_SUPPORT_LEGO_PROTOCOL == 1
    proto_lego,
#endif
#if IRMP_SUPPORT_THOMSON_PROTOCOL == 1
    proto_thomson,
#endif
#if IRMP_SUPPORT_BOSE_PROTOCOL == 1
    proto_bose,
#endif
#if IRMP_SUPPORT_A1TVBOX_PROTOCOL == 1
    proto_a1tvbox,
#endif
#if IRMP_SUPPORT_ORTEK_PROTOCOL == 1
    proto_ortek,
#endif
#if IRMP_SUPPORT_TELEFUNKEN_PROTOCOL == 1
    proto_telefunken,
#endif
#if IRMP_SUPPORT_ROOMBA_PROTOCOL == 1
    proto_roomba,
#endif
#if IRMP_SUPPORT_RCMM_PROTOCOL  == 1
    proto_rcmm32,
    proto_rcmm24,
    proto_rcmm12,
#endif
#if IRMP_SUPPORT_SPEAKER_PROTOCOL == 1
    proto_speaker,
#endif
#if IRMP_SUPPORT_LGAIR_PROTOCOL == 1
    proto_lgair,
#endif
#if IRMP_SUPPORT_SAMSUNG48_PROTOCOL == 1
    proto_samsung48,
#endif
#if IRMP_SUPPORT_MERLIN_PROTOCOL == 1
    proto_merlin,
#endif
#if IRMP_SUPPORT_PENTAX_PROTOCOL == 1
    proto_pentax,
#endif
#if IRMP_SUPPORT_FAN_PROTOCOL == 1
    proto_fan,
#endif
#if IRMP_SUPPORT_S100_PROTOCOL == 1
    proto_s100,
#endif
#if IRMP_SUPPORT_ACP24_PROTOCOL  == 1
    proto_acp24,
#endif
#if IRMP_SUPPORT_TECHNICS_PROTOCOL == 1
    proto_technics,
#endif
#if IRMP_SUPPORT_PANASONIC_PROTOCOL == 1
    proto_panasonic,
#endif
#if IRMP_SUPPORT_MITSU_HEAVY_PROTOCOL == 1
    proto_mitsu_heavy,
#endif
#if IRMP_SUPPORT_VINCENT_PROTOCOL == 1
    proto_vincent,
#endif
#if IRMP_SUPPORT_SAMSUNGAH_PROTOCOL == 1
    proto_samsungah,
#endif
#if IRMP_SUPPORT_IRMP16_PROTOCOL == 1
    proto_irmp16,
#endif
#if IRMP_SUPPORT_GREE_PROTOCOL == 1
    proto_gree,
#endif
#if IRMP_SUPPORT_RCII_PROTOCOL == 1
    proto_rcii,
#endif
#if IRMP_SUPPORT_METZ_PROTOCOL == 1
    proto_metz,
#endif
#if IRMP_SUPPORT_NEC_PROTOCOL == 1
    proto_onkyo,
#endif
#if IRMP_SUPPORT_RF_GEN24_PROTOCOL == 1
    proto_rf_gen24,
#endif
#if IRMP_SUPPORT_RF_X10_PROTOCOL == 1
    proto_rf_x10
#endif
};

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
