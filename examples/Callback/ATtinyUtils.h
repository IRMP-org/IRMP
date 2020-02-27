/*
 * ATtinyUtils.h
 *
 *  Copyright (C) 2018-2020  Armin Joachimsmeyer
 *  Email: armin.joachimsmeyer@gmail.com
 *
 *  This file is part of ArduinoUtils https://github.com/ArminJo/ArduinoUtils.
 *
 *  Arduino-Utils is free software: you can redistribute it and/or modify
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

//
// ATMEL ATTINY85
//
//                                        +-\/-+
//        PCINT5/!RESET/ADC0/dW (5) PB5  1|    |8  Vcc
// PCINT3/XTAL1/CLKI/!OC1B/ADC3 (3) PB3  2|    |7  PB2 (2) SCK/USCK/SCL/ADC1/T0/INT0/PCINT2
//  PCINT4/XTAL2/CLKO/OC1B/ADC2 (4) PB4  3|    |6  PB1 (1) MISO/DO/AIN1/OC0B/OC1A/PCINT1 / TX Debug output
//                                  GND  4|    |5  PB0 (0) MOSI/DI/SDA/AIN0/OC0A/!OC1A/AREF/PCINT0
//                                        +----+

// ATMEL ATTINY167
// Pin numbers are for Digispark core
// Pin numbers in parenthesis are for ATTinyCore
//
//                    +-\/-+
//    RX  6 (0) PA0  1|    |20  PB0 (D8)  0 OC1AU  TONE  Timer 1 Channel A
//    TX  7 (1) PA1  2|    |19  PB1 (9)  1 OC1BU  Internal LED
//        8 (2) PA2  3|    |18  PB2 (10) 2 OC1AV  Timer 1 Channel B
//   INT1 9 (3) PA3  4|    |17  PB3 (11) 4 OC1BV  connected with 51 Ohm to D- and 3.3 volt Zener.
//             AVCC  5|    |16  GND
//             AGND  6|    |15  VCC
//       10 (4) PA4  7|    |14  PB4 (12) XTAL1
//       11 (5) PA5  8|    |13  PB5 (13) XTAL2
//       12 (6) PA6  9|    |12  PB6 (14) 3 INT0  connected with 68 Ohm to D+ (and disconnected 3.3 volt Zener). Is terminated with ~20 kOhm if USB attached :-(
//        5 (7) PA7 10|    |11  PB7 (15) RESET
//                    +----+
//

#ifndef ATTINYUTILS_H_
#define ATTINYUTILS_H_

#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)

#include <Arduino.h>
#include <avr/io.h>

#if defined(ARDUINO_AVR_DIGISPARK)
#  ifndef LED_BUILTIN
#define LED_BUILTIN PB1
#  endif

#elif defined(ARDUINO_AVR_DIGISPARKPRO)
#undef LED_BUILTIN    // In case we use another core e.g. in Eclipse
#define LED_BUILTIN 1 // On a Digispark Pro we have PB1 / D1 (Digispark library) or D9 (ATtinyCore lib) / on DigisparkBoard labeled as pin 1
#endif

#if (F_CPU == 1000000)
#define TIMER0_CLOCK_DIVIDER_FOR_64_MICROS (_BV(CS01) | _BV(CS00))

#define TIMER1_CLOCK_DIVIDER_FOR_8_MICROS _BV(CS12)
#define TIMER1_CLOCK_DIVIDER_FOR_4_MICROS (_BV(CS11) | _BV(CS10))
#define TIMER1_CLOCK_DIVIDER_FOR_2_MICROS _BV(CS11)
#define TIMER1_CLOCK_DIVIDER_FOR_1_MICRO _BV(CS10)
#endif

#if (F_CPU == 8000000)
#define TIMER0_CLOCK_DIVIDER_FOR_128_MICROS (_BV(CS02) | _BV(CS00))

#define TIMER1_CLOCK_DIVIDER_FOR_8_MICROS (_BV(CS12) | _BV(CS11)| _BV(CS10))
#define TIMER1_CLOCK_DIVIDER_FOR_4_MICROS (_BV(CS12) | _BV(CS11))
#define TIMER1_CLOCK_DIVIDER_FOR_2_MICROS (_BV(CS12) | _BV(CS10))
#define TIMER1_CLOCK_DIVIDER_FOR_1_MICRO _BV(CS12)
#endif

/*
 * Formula is only valid for constant values
 * Loading of constant value adds 2 extra cycles (check .lss file for exact timing)
 *
 * Only multiple of 4 cycles are possible. Last loop is only 3 cycles.
 * 1 -> 3(+2) cycles
 * 2 -> 7(+2) cycles
 * 3 -> 11(+2) cycles
 * 4 -> 15(+2) cycles
 * 5 -> 19(+2) cycles
 * 6 -> 23(+2) cycles
 */
inline void delay4CyclesInlineExact(uint16_t a4Microseconds) {
    // the following loop takes 4 cycles (4 microseconds  at 1 MHz) per iteration
    __asm__ __volatile__ (
            "1: sbiw %0,1" "\n\t"    // 2 cycles
            "brne .-4" : "=w" (a4Microseconds) : "0" (a4Microseconds)// 2 cycles
    );
}
#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
void toneWithTimer1PWM(uint16_t aFrequency, bool aUseOutputB = false);
#endif

uint8_t getBODLevelFuses();
bool isBODLevelBelow2_7Volt();
bool isBODSFlagExistent();
void changeDigisparkClock();

#endif //  defined (__AVR_ATtiny85__)
#endif /* ATTINYUTILS_H_ */

#pragma once
