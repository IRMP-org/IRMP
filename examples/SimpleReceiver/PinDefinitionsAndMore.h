/*
 *  PinDefinitionsAndMore.h
 *
 *  Contains pin definitions for IRMP examples for various platforms
 *  as well as definitions for feedback LED and tone() and includes
 *
 *  Copyright (C) 2020-2021  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of IRMP https://github.com/IRMP-org/IRMP.
 *
 *  IRMP is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

/*
 * Pin mapping table for different platforms
 *
 * Platform     IR input    IR output   Tone      Core/Pin schema
 * --------------------------------------------------------------
 * DEFAULT/AVR  2           3           4
 * ATtinyX5     0|PB0       4|PB4       3|PB3
 * ATtiny167    3|PA3       2|PA2       7|PA7     ATTinyCore
 * ATtiny167    9|PA3       8|PA2       5|PA7     Digispark pro
 * ATtiny3216  14|PA1      15|PA2      16|PA3     MegaTinyCore
 * SAMD21       3           4           5
 * ESP8266     14|D5       12|D6        %
 * ESP32       15           4           %
 * ESP32-C3     6           7          10
 * BluePill   PA6         PA7         PA3
 * APOLLO3     11          12           5
 * RP2040       3|GPIO15    4|GPIO16    5|GPIO17
 */
//#define IRMP_MEASURE_TIMING // For debugging purposes.
//
#if defined(ESP8266)
#define FEEDBACK_LED_IS_ACTIVE_LOW // The LED on my board is active LOW
#define IRMP_INPUT_PIN      14 // D5
#define IRSND_OUTPUT_PIN    12 // D6 - D4/2 is internal LED
#define IR_TIMING_TEST_PIN  13 // D7

#define tone(...) void()      // tone() inhibits receive timer
#define noTone(a) void()
#define TONE_PIN            42 // Dummy for examples using it

#elif defined(CONFIG_IDF_TARGET_ESP32C3) || defined(ARDUINO_ESP32C3_DEV)
#define NO_LED_FEEDBACK_CODE   // The  WS2812 on pin 8 of AI-C3 board crashes if used as receive feedback LED, other I/O pins are working...
#define IRMP_INPUT_PIN       6
#define IRSND_OUTPUT_PIN     7
#define TONE_PIN            10

#elif defined(ESP32)
#define IRMP_INPUT_PIN      15  // D15
#define IRSND_OUTPUT_PIN     4  // D4
#include <Arduino.h>
#define TONE_LEDC_CHANNEL        1  // Using channel 1 makes tone() independent of receiving timer -> No need to stop receiving timer.
// tone() is included in ESP32 core since 2.0.2
#if !defined(ESP_ARDUINO_VERSION_VAL)
#define ESP_ARDUINO_VERSION_VAL(major, minor, patch) 12345678
#endif
#if ESP_ARDUINO_VERSION  <= ESP_ARDUINO_VERSION_VAL(2, 0, 2)
#define TONE_LEDC_CHANNEL        1  // Using channel 1 makes tone() independent of receiving timer -> No need to stop receiving timer.
void tone(uint8_t aPinNumber, unsigned int aFrequency){
    ledcAttachPin(aPinNumber, TONE_LEDC_CHANNEL);
    ledcWriteTone(TONE_LEDC_CHANNEL, aFrequency);
}
void tone(uint8_t aPinNumber, unsigned int aFrequency, unsigned long aDuration){
    ledcAttachPin(aPinNumber, TONE_LEDC_CHANNEL);
    ledcWriteTone(TONE_LEDC_CHANNEL, aFrequency);
    delay(aDuration);
    ledcWriteTone(TONE_LEDC_CHANNEL, 0);
}
void noTone(uint8_t aPinNumber){
    ledcWriteTone(TONE_LEDC_CHANNEL, 0);
}
#endif // ESP_ARDUINO_VERSION  <= ESP_ARDUINO_VERSION_VAL(2, 0, 2)
#define TONE_PIN                27  // D27 25 & 26 are DAC0 and 1

#elif defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_ARCH_STM32F1)
// BluePill in 2 flavors
// Timer 3 of IRMP blocks PA6, PA7, PB0, PB1 for use by Servo or tone()
#define FEEDBACK_LED_IS_ACTIVE_LOW // The LED on the BluePill is active LOW
#define IRMP_INPUT_PIN          PA6
#define IRSND_OUTPUT_PIN        PA7
#define TONE_PIN                PA3
#define IR_TIMING_TEST_PIN      PA5

#elif  defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#include "ATtinySerialOut.hpp" // Available as Arduino library "ATtinySerialOut"
#define IRMP_INPUT_PIN   0
#define IRSND_OUTPUT_PIN 4 // Pin 2 is serial output with ATtinySerialOut. Pin 1 is internal LED and Pin3 is USB+ with pullup on Digispark board.
#define TONE_PIN         3
//#define IR_TIMING_TEST_PIN 3

#elif defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
#include "ATtinySerialOut.hpp" // Available as Arduino library "ATtinySerialOut"
// For ATtiny167 Pins PB6 and PA3 are usable as interrupt source.
#  if defined(ARDUINO_AVR_DIGISPARKPRO)
#define IRMP_INPUT_PIN   9 // PA3 - on Digispark board labeled as pin 9
//#define IRMP_INPUT_PIN  14 // PB6 / INT0 is connected to USB+ on DigisparkPro boards
#define IRSND_OUTPUT_PIN 8 // PA2 - on Digispark board labeled as pin 8
#define TONE_PIN         5 // PA7
#define IR_TIMING_TEST_PIN 10 // PA4

#  else
#define IRMP_INPUT_PIN   3
#define IRSND_OUTPUT_PIN 2
#define TONE_PIN         7
#  endif

#elif defined(__AVR_ATtiny88__) // MH-ET Tiny88 board
#include "ATtinySerialOut.hpp" // Available as Arduino library "ATtinySerialOut"
// Pin 6 is TX pin 7 is RX
#define IRMP_INPUT_PIN   3 // INT1
#define IRSND_OUTPUT_PIN 4
#define TONE_PIN         9
#define IR_TIMING_TEST_PIN 8

#elif defined(__AVR_ATtiny1616__)  || defined(__AVR_ATtiny3216__) || defined(__AVR_ATtiny3217__)
#define IRMP_INPUT_PIN   PIN_PA1 // 14 use 18 instead of PIN_PA1 for TinyCore32
#define IRSND_OUTPUT_PIN PIN_PA2 // 15, 19 for TinyCore32
#define TONE_PIN         PIN_PA3 // 16, 20 for TinyCore32

#elif defined(ARDUINO_ARCH_APOLLO3)
#define IRMP_INPUT_PIN   11
#define IRSND_OUTPUT_PIN 12
#define TONE_PIN         5

#elif defined(ARDUINO_ARCH_MBED) && defined(ARDUINO_ARCH_MBED_NANO) // Arduino Nano 33 BLE, Arduino Nano RP2040 Connect
#define IRMP_INPUT_PIN      3   // GPIO15 Start with pin 3 since pin 2|GPIO25 is connected to LED on Pi pico
#define IRSND_OUTPUT_PIN    4   // GPIO16
#define TONE_PIN            5

#elif defined(ARDUINO_ARCH_RP2040) // Pi Pico with arduino-pico core https://github.com/earlephilhower/arduino-pico
#define IRMP_INPUT_PIN      15  // to be compatible with the Arduino Nano RP2040 Connect (pin3)
#define IRSND_OUTPUT_PIN    16
#define TONE_PIN            17

// If you program the Nano RP2040 Connect with this core, then you must redefine LED_BUILTIN
// and use the external reset with 1 kOhm to ground to enter UF2 mode
#undef LED_BUILTIN
#define LED_BUILTIN          6

#elif defined(TEENSYDUINO)
#define IRMP_INPUT_PIN      2
#define IRSND_OUTPUT_PIN    3
#define TONE_PIN            4
#if !defined(ALTERNATIVE_IR_FEEDBACK_LED_PIN)
#define ALTERNATIVE_IR_FEEDBACK_LED_PIN 6 // E.g. used for examples which use LED_BUILDIN for example output.
#endif
#define IR_TIMING_TEST_PIN  7

#elif defined(__AVR__) // Standard AVR Boards like Uno, Nano
#define IRMP_INPUT_PIN      2 // To be compatible with interrupt example, pin 2 is chosen here.
#define IRSND_OUTPUT_PIN    3
#define TONE_PIN            4
#if !defined(ALTERNATIVE_IR_FEEDBACK_LED_PIN)
#define ALTERNATIVE_IR_FEEDBACK_LED_PIN 6 // E.g. used for examples which use LED_BUILDIN for example output.
#endif
#define IR_TIMING_TEST_PIN 7
// You can alternatively specify the input pin with port and bit number if you do not have the Arduino pin number at hand
//#define IRMP_PORT_LETTER D
//#define IRMP_BIT_NUMBER 2

#elif defined(ARDUINO_ARCH_SAMD)
#define IRMP_INPUT_PIN      2
#define IRSND_OUTPUT_PIN    3
#define TONE_PIN            4
#if !defined(ALTERNATIVE_IR_FEEDBACK_LED_PIN)
#define ALTERNATIVE_IR_FEEDBACK_LED_PIN 6 // E.g. used for examples which use LED_BUILDIN for example output.
#endif
#define IR_TIMING_TEST_PIN  7

// On the Zero and others we switch explicitly to SerialUSB
#define Serial SerialUSB

// Definitions for the Chinese SAMD21 M0-Mini clone, which has no led connected to D13/PA17.
// Attention!!! D2 and D4 are swapped on these boards!!!
// If you connect the LED, it is on pin 24/PB11. In this case activate the next two lines.
//#undef LED_BUILTIN
//#define LED_BUILTIN 24 // PB11
// As an alternative you can choose pin 25, it is the RX-LED pin (PB03), but active low.In this case activate the next 3 lines.
//#undef LED_BUILTIN
//#define LED_BUILTIN 25 // PB03
//#define FEEDBACK_LED_IS_ACTIVE_LOW // The RX LED on the M0-Mini is active LOW

#else
#warning Board / CPU is not detected using pre-processor symbols -> using default values, which may not fit. Please extend PinDefinitionsAndMore.h.
// Default valued for unidentified boards
#define IRMP_INPUT_PIN      2
#define IRSND_OUTPUT_PIN    3
#define TONE_PIN            4
#if !defined(ALTERNATIVE_IR_FEEDBACK_LED_PIN)
#define ALTERNATIVE_IR_FEEDBACK_LED_PIN 6 // E.g. used for examples which use LED_BUILDIN for example output.
#endif
#define IR_TIMING_TEST_PIN  7
#endif // defined(ESP8266)


#if defined(__AVR_ATmega4809__) // for standard AVR we manage hardware directly in void enablePCIInterrupt()
#define IRMP_USE_ARDUINO_ATTACH_INTERRUPT
#endif

/*
 * Helper macro for getting a macro definition as string
 */
#if !defined(STR_HELPER)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#endif
