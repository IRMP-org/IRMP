/*
 *  SimpleSender.cpp
 *
 *  Sends Samsung protocol frames.
 *  Is able to send IR protocol data of 15 main protocols.
 *
 *      Sony SIRCS
 *      NEC + APPLE
 *      Samsung + Samsg32
 *      Kaseikyo
 *
 *      Plus 11 other main protocols
 *      JVC, NEC16, NEC42, Matsushita, DENON, Sharp, RC5, RC6 & RC6A, IR60 (SDA2008) Grundig, Siemens Gigaset, Nokia
 *
 *  To disable one of them or to enable other protocols, specify this before the "#include <irmp.c.h>" line.
 *  If you get warnings of redefining symbols, just ignore them or undefine them first (see Interrupt example).
 *  The exact names can be found in the library file irmpSelectAllProtocols.h (see Callback example).
 *
 *  Copyright (C) 2019-2020  Armin Joachimsmeyer
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

#include <Arduino.h>

#define VERSION_EXAMPLE "1.2"

/*
 * Set library modifiers first to set output pin etc.
 */
#if defined(ESP8266)
#define BLINK_13_LED_IS_ACTIVE_LOW // The LED on my board is active LOW
#define IRSND_OUTPUT_PIN 12 // D6

#elif defined(ESP32)
#define IRSND_OUTPUT_PIN  4  // D4

#elif defined(STM32F1xx) || defined(__STM32F1__)
// BluePill in 2 flavors
// STM32F1xx is for "Generic STM32F1 series" from STM32 Boards from STM32 cores of Arduino Board manager
// __STM32F1__is for "Generic STM32F103C series" from STM32F1 Boards (STM32duino.com) of manual installed hardware folder
#define BLINK_13_LED_IS_ACTIVE_LOW // The LED on the BluePill is active LOW
#define IRSND_OUTPUT_PIN PA7

#elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
#include "ATtinySerialOut.h"
#  if  defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#define IRSND_OUTPUT_PIN 4 // Pin 2 is serial output with ATtinySerialOut. Pin 1 is internal LED and Pin3 is USB+ with pullup on Digispark board.
#    if defined(ARDUINO_AVR_DIGISPARK)
#define LED_BUILTIN PB1
#    endif

#  else
#    if defined(ARDUINO_AVR_DIGISPARKPRO)
#define LED_BUILTIN 1 // On a Digispark Pro we have PB1 / D1 (Digispark library) or D9 (ATtinyCore lib) / on DigisparkBoard labeled as pin 1
#define IRSND_OUTPUT_PIN 8  // PA2 - on DigisparkBoard labeled as pin 8
#    else
#define IRSND_OUTPUT_PIN 2
#    endif
#  endif

#else
#define IRSND_OUTPUT_PIN 4
#endif

// On the Zero and others we switch explicitly to SerialUSB
#if defined(ARDUINO_ARCH_SAMD)
#define Serial SerialUSB
#endif

//#define IR_OUTPUT_IS_ACTIVE_LOW

#include <irsndSelectMain15Protocols.h>
/*
 * After setting the modifiers we can include the code and compile it.
 */
#include <irsnd.c.h>

IRMP_DATA irsnd_data;

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

void setup()
{
    Serial.begin(115200);
#if defined(__AVR_ATmega32U4__)
    while (!Serial); //delay for Leonardo, but this loops forever for Maple Serial
#endif
#if defined(SERIAL_USB)
    delay(2000); // To be able to connect Serial monitor after reset and before first printout
#endif
#if defined(__ESP8266__)
	Serial.println(); // to separate it from the internal boot output
#endif

    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ "\r\nVersion " VERSION_EXAMPLE " from " __DATE__));
    irsnd_init();
    irsnd_blink13(true); // Enable LED feedback

#if defined(STM32F1xx)
    Serial.println(F("Ready to send IR signals at pin PA5")); // the internal pin numbers are crazy for the STM32 Boards library
#else
    Serial.println(F("Ready to send IR signals at pin " STR(IRSND_OUTPUT_PIN)));
#endif
    irsnd_data.protocol = IRMP_SAMSUNG32_PROTOCOL;
    irsnd_data.address = 0x0707;
    irsnd_data.command = 0xFB04;
    irsnd_data.flags = 0; // repeat frame 0 time

    Serial.print(F("Send 0x"));
    Serial.println(irsnd_data.command,HEX);
    irsnd_send_data(&irsnd_data, false);

}

void loop()
{
    delay(5000);
    uint8_t tNextCommand = irsnd_data.command;
    tNextCommand++;
    // For my Samsung the high byte is the negative of the low byte
    irsnd_data.command = ((~tNextCommand) << 8) | tNextCommand;
    Serial.print(F("Send 0x"));
    Serial.println(irsnd_data.command,HEX);
    irsnd_send_data(&irsnd_data, false); // This stores timer state and restores it after sending
}
