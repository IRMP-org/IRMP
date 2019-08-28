/*
 *  SimpleReceiver.cpp
 *
 *  Receives IR protocol data of 15 main protocols.
 *
 *  *****************************************************************************************************************************
 *  To access the library files from your sketch, you have to first use `Sketch/Show Sketch Folder (Ctrl+K)` in the Arduino IDE.
 *  Then navigate to the parallel `libraries` folder and select the library you want to access.
 *  The library files itself are located in the `src` sub-directory.
 *  If you did not yet store the example as your own sketch, then with Ctrl+K you are instantly in the right library folder.
 *  *****************************************************************************************************************************
 *
 *
 *  The following IR protocols are enabled by default:
 *      Sony SIRCS
 *      NEC + APPLE
 *      Samsung + Samsg32
 *      Kaseikyo
 *
 *      Plus 11 other main protocols by including irmpMain15.h instead of irmp.h
 *      JVC, NEC16, NEC42, Matsushita, DENON, Sharp, RC5, RC6 & RC6A, IR60 (SDA2008) Grundig, Siemens Gigaset, Nokia
 *
 *  To disable one of them or to enable other protocols, specify this before the "#include <irmp.h>" line.
 *  If you get warnings of redefining symbols, just ignore them or undefine them first (see Interrupt example).
 *  The exact names can be found in the library file irmpSelectAllProtocols.h (see Callback example).
 *
 *  Copyright (C) 2019  Armin Joachimsmeyer
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

#define VERSION_EXAMPLE "1.1"

/*
 * Set library modifiers first to set input pin etc.
 */
#if defined(ESP8266)
#define IRMP_INPUT_PIN 14 // D5
#elif defined(ESP32)
#define IRMP_INPUT_PIN 15
#else
#define IRMP_INPUT_PIN 3
#endif

#define IRMP_PROTOCOL_NAMES 1 // Enable protocol number mapping to protocol strings - needs some FLASH. Must before #include <irmp*>

#include <irmpSelectMain15Protocols.h>  // This enables 15 main protocols

/*
 * After setting the modifiers we can include the code and compile it.
 */
#include <irmp.c.h>

IRMP_DATA irmp_data[1];

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

void setup() {
// initialize the digital pin as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
    while (!Serial)
        ; //delay for Leonardo
    // Just to know which program is running on my Arduino
#if defined(__ESP8266__)
    Serial.println();
#endif
    Serial.println(F("START " __FILE__ "\r\nVersion " VERSION_EXAMPLE " from " __DATE__));
    irmp_init();
    irmp_blink13(true); // Enable LED feedback

    Serial.println(F("Ready to receive IR signals at pin " STR(IRMP_INPUT_PIN)));
}

void loop() {
    if (irmp_get_data(&irmp_data[0])) {

        switch (irmp_data[0].command) {
        case 0x48:
            digitalWrite(LED_BUILTIN, HIGH);
            break;
        case 0x0B:
            digitalWrite(LED_BUILTIN, LOW);
            break;
        default:
            break;
        }

        Serial.print(F("P="));
#if defined(__AVR__)
        const char* tProtocolStringPtr = (char*) pgm_read_word(&irmp_protocol_names[irmp_data[0].protocol]);
        Serial.print((__FlashStringHelper *) (tProtocolStringPtr));
#else
        Serial.print(irmp_protocol_names[irmp_data[0].protocol]);
#endif
        Serial.print(F(" "));
        Serial.print(F(" A=0x"));
        Serial.print(irmp_data[0].address, HEX);
        Serial.print(F(" C=0x"));
        Serial.print(irmp_data[0].command, HEX);
        if (irmp_data[0].flags & IRMP_FLAG_REPETITION) {
            Serial.print(F(" R"));
        }
        Serial.println();
    }
}
