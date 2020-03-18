/*
 * ATtinyUtils.cpp
 *
 *  Copyright (C) 2016-2020  Armin Joachimsmeyer
 *  Email: armin.joachimsmeyer@gmail.com
 *
 *  This file is part of Arduino-Utils https://github.com/ArminJo/Arduino-Utils.
 *
 *  ArduinoUtils is free software: you can redistribute it and/or modify
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
#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)

//#define DEBUG
#ifdef DEBUG
// Should be first include to avoid unwanted use of Serial object defined in HardwareSerial
#include "ATtinySerialOut.h"
#endif

#include "ATtinyUtils.h"

#include <avr/boot.h>  // required for boot_signature_byte_get()
#include <avr/power.h> // required for clock_prescale_set()
#include <avr/sleep.h> // required for isBODSFlagExistent()
#include "digitalWriteFast.h"

// since we have not included Arduino.h
#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

/*
 * Use port pin number (PB0-PB5) not case or other pin number
 */
inline void toggleFastPortB(uint8_t aOutputPinNumber) {
    PINB = (1 << aOutputPinNumber);
}

// for constant values we can also use: digitalWriteFast()
inline void digitalWriteFastPortB(uint8_t aOutputPinNumber, bool aValue) {
    (aValue ? PORTB |= (1 << aOutputPinNumber) : PORTB &= ~(1 << aOutputPinNumber));
}

// for constant values we can also use: digitalReadFast()
inline bool digitalReadFastPortB(uint8_t aInputPinNumber) {
    return (PINB & (1 << aInputPinNumber));
}

// not for INPUT_PULLUP - can be done by setting to input and adding digitalWriteFastPortB(aOutputPinNumber,1);
inline void pinModeFastPortB(uint8_t aOutputPinNumber, uint8_t aMode) {
    (aMode ? DDRB |= (1 << aOutputPinNumber) /* OUTPUT */: DDRB &= ~(1 << aOutputPinNumber));
}

#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)

/*
 * initialize outputs and use PWM Mode
 * if aUseOutputB == false output frequency at Pin6/5 - PB1/PB0 - OCR1A/!OCR1A
 * else at Pin3/2 - PB4/PB3 - OCR1B/!OCR1B
 */
void toneWithTimer1PWM(uint16_t aFrequency, bool aUseOutputB) {
    uint8_t tPrescaler = 0x01;
    uint16_t tOCR = F_CPU / aFrequency;
    while (tOCR > 0x100 && tPrescaler < 0x0F) {
        tPrescaler++;
        tOCR >>= 1;
    }

    OCR1C = tOCR - 1; // The frequency of the PWM will be Timer Clock 1 frequency divided by (OCR1C value + 1).

    if (aUseOutputB) {
        OCR1B = tOCR / 2; // 50% PWM
        pinModeFast(PB3, OUTPUT);
        pinModeFast(PB4, OUTPUT);
        GTCCR = (1 << PWM1B) | (1 << COM1B0); // PWM Mode with OCR1B (PB4) + !OCR1B (PB3) outputs enabled
        TCCR1 = tPrescaler;
    } else {
        OCR1A = tOCR / 2; // 50% PWM
        pinModeFast(PB1, OUTPUT);
        pinModeFast(PB0, OUTPUT);
        GTCCR = 0;
        TCCR1 = (1 << PWM1A) | (1 << COM1A0) | tPrescaler; // PWM Mode with OCR1A (PB1) + !OCR1A (PB0) outputs enabled
    }
}

void periodicInterruptWithTimer1(uint16_t aFrequency){
    uint8_t tPrescaler = 0x01;
    uint16_t tOCR = F_CPU / aFrequency;
    while (tOCR > 0x100 && tPrescaler < 0x0F) {
        tPrescaler++;
        tOCR >>= 1;
    }
    OCR1C = tOCR - 1; // The frequency of the interrupt will be Timer Clock 1 frequency divided by (OCR1C value + 1).

    TCCR1 |= (1 << CTC1) | tPrescaler;  // clear timer on compare match
    TIMSK |= (1 << OCIE1A); // enable compare match interrupt
}
#endif

uint8_t getBODLevelFuses() {
    return boot_lock_fuse_bits_get(GET_HIGH_FUSE_BITS) & (~FUSE_BODLEVEL2 | ~FUSE_BODLEVEL1 | ~FUSE_BODLEVEL0 );
}

bool isBODLevelBelow2_7Volt() {
    return (getBODLevelFuses() >= 6);
}

/*
 * @return true if BODS flag is existent - should be true for ATtiny85 revision C and later
 */
bool isBODSFlagExistent() {
    sleep_bod_disable()
    ;
    /*
     * Check if flag can be set - this works only for ATtini85 revision C, which is quite unpopular (2019) on Chinese boards.
     */
    return MCUCR & _BV(BODS);
}

/*
 * Code to change Digispark Bootloader clock settings to get the right CPU frequency
 * and to reset Digispark OCCAL tweak.
 * Call it if you want to use the standard ATtiny library, BUT do not call it, if you need Digispark USB functions available for 16 MHz.
 */
void changeDigisparkClock() {
    uint8_t tLowFuse = boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS);
#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    if ((tLowFuse & (~FUSE_CKSEL3 | ~FUSE_CKSEL2 | ~FUSE_CKSEL1 | ~FUSE_CKSEL0 )) == 0x01) { // cannot use ~FUSE_CKSEL0 on right side :-(
#elif defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
    if ((tLowFuse & (~FUSE_CKSEL3 | ~FUSE_CKSEL2 | ~FUSE_CKSEL1 )) == 0x0E) { // cannot use ~FUSE_CKSEL1 on right side :-(
#endif
        /*
         * Here we have High Frequency PLL Clock ( 16 MHz)
         */
#if (F_CPU == 1000000)
        // Divide 16 MHz Pll clock by 16 for Digispark Boards to get the requested 1 MHz
        clock_prescale_set(clock_div_16);
//        CLKPR = (1 << CLKPCE);  // unlock function
//        CLKPR = (1 << CLKPS2); // 0x04 -> %16
#endif
#if (F_CPU == 8000000)
        // Divide 16 MHz Pll clock by 2 for Digispark Boards to get the requested 8 MHz
        clock_prescale_set(clock_div_2);
//        CLKPR = (1 << CLKPCE);  // unlock function
//        CLKPR = (1 << CLKPS0); // 0x01 -> %2
#endif
    }

    /*
     * Code to reset Digispark OCCAL tweak
     */
#define  SIGRD  5 // required for boot_signature_byte_get()
    uint8_t tStoredOSCCAL = boot_signature_byte_get(1);
    if (OSCCAL != tStoredOSCCAL) {
#ifdef DEBUG
        uint8_t tOSCCAL = OSCCAL;
        writeString(F("Changed OSCCAL from "));
        writeUnsignedByteHex(tOSCCAL);
        writeString(F(" to "));
        writeUnsignedByteHex(tStoredOSCCAL);
        writeCRLF();
#endif
        // retrieve the factory-stored oscillator calibration bytes to revert the Digispark OSCCAL tweak
        OSCCAL = tStoredOSCCAL;
    }
}

#endif //  defined (__AVR_ATtiny85__)
