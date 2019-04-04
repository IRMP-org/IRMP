/*
 *  Interrupt.cpp
 *
 *  Receives IR protocol data at pin 3.
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
 *  To disable one of them or to enable other protocols, you must specify this around line 62 after the "#include <irmp.h>" and before "#include <irmp.cpp.h>".
 *  If you get warnings just ignore them. You can avoid them, if you modify the library file irmpconfig.h directly to specify the protocols.
 *  The exact names of the modifiers can be found in the library file irmpconfig.h at line 50ff.
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

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#include <Arduino.h>

#define VERSION_EXAMPLE "1.0"

/*
 * Set library modifiers first to set input pin etc.
 */
#define IRMP_INPUT_PIN 3

#define IRMP_PROTOCOL_NAMES              1 // Enable protocol number mapping to protocol strings - needs some FLASH
#define IRMP_USE_COMPLETE_CALLBACK       1 // Enable callback functionality
#define IRMP_ENABLE_PIN_CHANGE_INTERRUPT 1 // Enable interrupt functionality

//#define SIZE_TEST
#ifdef SIZE_TEST
#include <irmpNone.h>
#define IRMP_SUPPORT_NEC_PROTOCOL        1
#else
#include <irmpMain15.h>  // This enables 15 main protocols
// this protocols is incompatible to NEC in interrupt mode, since it is the same as NEC but has longer data sections
#define IRMP_SUPPORT_NEC42_PROTOCOL      0
#endif

/*
 * After setting the modifiers we can include the code and compile it.
 */
#include <irmp.c.h>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

void printReceivedIRData();
void initPCIInterrupt();

// local modifiers
//#define IRMP_USE_ARDUINO_ATTACH_INTERRUPT

void setup() {
// initialize the digital pin as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
    while (!Serial)
        ; //delay for Leonardo
    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ "\r\nVersion " VERSION_EXAMPLE " from " __DATE__));
    //Enable auto resume and pass it the address of your extra buffer
    irmp_init();
#ifdef IRMP_USE_ARDUINO_ATTACH_INTERRUPT
    attachInterrupt(digitalPinToInterrupt(IRMP_BIT_NUMBER), irmp_PCI_ISR, CHANGE);
#else
    initPCIInterrupt();
#endif
    irmp_register_complete_callback_function(&printReceivedIRData);

    Serial.println(F("Ready to receive IR signals at pin " STR(IRMP_INPUT_PIN)));
}

IRMP_DATA irmp_data[1];

void loop() {
#ifndef SIZE_TEST
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
#endif
}

/*
 * Here we know, that data is available.
 * Since this function is executed in Interrupt handler context, make it short and do not use delay() etc.
 * In order to enable other interrupts you can call sei() (enable interrupt again) after getting data.
 */
void printReceivedIRData() {
    irmp_get_data(&irmp_data[0]);
    // enable interrupts
    sei();
#ifndef SIZE_TEST
    Serial.print(F("P="));

    /*
     * To see full ASCII protocol names, you must modify irmpconfig.h line 227.
     * Use `Sketch/Show Sketch Folder (Ctrl+K)` in the Arduino IDE and the instructions above to access it.
     */
#if IRMP_PROTOCOL_NAMES == 1
    const char* tProtocolStringPtr = (char*) pgm_read_word(&irmp_protocol_names[irmp_data[0].protocol]);
    Serial.print((__FlashStringHelper *) (tProtocolStringPtr));
    Serial.print(F(" "));
#else
    Serial.print(F("0x"));
    Serial.print(irmp_data[0].protocol, HEX);
#endif
#endif
    Serial.print(F(" A=0x"));
    Serial.print(irmp_data[0].address, HEX);
    Serial.print(F(" C=0x"));
    Serial.print(irmp_data[0].command, HEX);
#ifndef SIZE_TEST
    if (irmp_data[0].flags & IRMP_FLAG_REPETITION) {
        Serial.print(F(" R"));
    }
#endif
    Serial.println();
}
