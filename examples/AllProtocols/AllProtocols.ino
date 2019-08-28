/*
 *  AllProtocols.cpp
 *
 *  Accepts 40 protocols simultaneously
 *
 *  Uses a callback function which is called every time a complete IR command was received.
 *  Prints data to LCD connected at pin 4-9
 *
 *  *****************************************************************************************************************************
 *  To access the library files from your sketch, you have to first use `Sketch/Show Sketch Folder (Ctrl+K)` in the Arduino IDE.
 *  Then navigate to the parallel `libraries` folder and select the library you want to access.
 *  The library files itself are located in the `src` sub-directory.
 *  If you did not yet store the example as your own sketch, then with Ctrl+K you are instantly in the right library folder.
 *  *****************************************************************************************************************************
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

#include <LiquidCrystal.h>

#define LCD_COLUMNS 16
#define LCD_ROWS 2

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

#define IRMP_PROTOCOL_NAMES              1 // Enable protocol number mapping to protocol strings - needs some FLASH
#define IRMP_USE_COMPLETE_CALLBACK       1 // Enable callback functionality

#define F_INTERRUPTS                     20000 // Instead of default 15000 to support LEGO + RCMM protocols, but this in turn disables PENTAX and GREE protocols :-(

#include <irmpSelectAllProtocols.h>  // This enables 15 main protocols

/*
 * After setting the modifiers we can include the code and compile it.
 */
#include <irmp.c.h>

IRMP_DATA irmp_data[1];

LiquidCrystal myLCD(4, 5, 6, 7, 8, 9);

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

void handleReceivedIRData();

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
    irmp_blink13(true); // Enable LED feedback
    irmp_register_complete_callback_function(&handleReceivedIRData);

    Serial.println(F("Ready to receive IR signals at pin " STR(IRMP_INPUT_PIN)));

    myLCD.begin(LCD_COLUMNS, LCD_ROWS);
    myLCD.print(F("IRMP all V" VERSION_EXAMPLE));
    myLCD.setCursor(0, 1);
    myLCD.print(F(__DATE__));
}

void loop() {
}

/*
 * Here we know, that data is available.
 * Since this function is executed in Interrupt handler context, make it short and do not use delay() etc.
 * In order to enable other interrupts you can call sei() (enable interrupt again) after getting data.
 */
void handleReceivedIRData() {
    /*
     * Just print the data to Serial and LCD
     */

    irmp_get_data(&irmp_data[0]);
    // enable interrupts
    sei();

    /*
     * Serial output
     */
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

    /*
     * LCD output
     */
    // Show protocol name
    myLCD.clear();
    myLCD.setCursor(0, 0);
    myLCD.print(F("P="));
#if defined(__AVR__)
    myLCD.print((__FlashStringHelper *) (tProtocolStringPtr));
#else
    myLCD.print(irmp_protocol_names[irmp_data[0].protocol]);
#endif
    // Show address
    myLCD.setCursor(0, 1);
    myLCD.print(F("A=0x"));
    myLCD.print(irmp_data[0].address, HEX);

    // Show command
    uint16_t tCommand = irmp_data[0].command;
    myLCD.setCursor(10, 1);
#if (LCD_COLUMNS <= 16)
    if (tCommand >= 0x100) {
        myLCD.print(F("0x"));
    } else {
        myLCD.print(F("C=0x"));
    }
#else
    myLCD.print(F("C=0x"));

#endif
    myLCD.print(tCommand, HEX);
}
