/*
 *  SimpleReceiver.cpp
 *
 *  Receives IR protocol data at pin 4. You can change this at line 113 in irmpconfig.h
 *
 *  *****************************************************************************************************************************
 *  To access the library files from your sketch, you have to first use `Sketch/Show Sketch Folder (Ctrl+K)` in the Arduino IDE.
 *  Then navigate to the parallel `libraries` folder and select the library you want to access.
 *  The library files itself are located in the `src` sub-directory.
 *  If you did not yet store the example as your own sketch, then with Ctrl+K you are instantly in the right library folder.
 *  *****************************************************************************************************************************
 *
 *  For this example you need to install "TimerOne" library with *Sketch -> Include Library -> Manage Librarys...*.
 *  Use "TimerOne" as filter string.
 *
 *  The following IR protocols are enabled by default:
 *      Sony SIRCS
 *      NEC + APPLE
 *      Samsung + Samsg32
 *      Kaseikyo
 *  To disable one of them or to enable other protocols, you must modify the library file irmpconfig.h at line 50ff.
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

#include <TimerOne.h>

#define VERSION_EXAMPLE "1.0"

#include <irmp.h>

void timerinterrupt();

// F_INTERRUPTS = 15000 Interrupts per second
#define TIMER_MICROSECONDS (1000000 / F_INTERRUPTS) // is 66 at F_INTERRUPTS = 15000

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

    Timer1.initialize(TIMER_MICROSECONDS);
    Timer1.attachInterrupt(timerinterrupt);
    Serial.println(F("Ready to receive IR signals"));
}

IRMP_DATA irmp_data[1];

void loop() {
    if (irmp_get_data(&irmp_data[0])) {
        // Feedback received data by LED
        digitalWrite(LED_BUILTIN, HIGH);

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
        Serial.print(F(" A=0x"));
        Serial.print(irmp_data[0].address, HEX);
        Serial.print(F(" C=0x"));
        Serial.print(irmp_data[0].command, HEX);
        Serial.print(F(" F=0x"));
        Serial.print(irmp_data[0].flags, HEX);
        Serial.println();

        digitalWrite(LED_BUILTIN, LOW);

        irmp_data[0].flags = 0;    // reset flags!
    }
}

// helper function: attachInterrupt wants void(), but irmp_ISR is uint8_t()
void timerinterrupt() {
    irmp_ISR();
}
