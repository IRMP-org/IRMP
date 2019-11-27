/*
 *  AllProtocols.cpp
 *
 *  Accepts 40 protocols simultaneously
 *  If you specify F_INTERRUPTS to 20000 at line 86 (default is 15000) it supports LEGO + RCMM protocols, but disables PENTAX and GREE protocols.
 *  if you see performance issues, you can disable MERLIN Protocol at line 88.
 *
 *  Uses a callback function which is called every time a complete IR command was received.
 *  Prints data to LCD connected parallel at pin 4-9 or serial at pin A4, A5
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

/*
 * Comment out, what LCD you use
 */
//#define USE_PARALELL_LCD
//#define USE_SERIAL_LCD
/*
 * Define the size of your LCD
 */
//#define USE_1602_LCD
//#define USE_2004_LCD

#if defined (USE_SERIAL_LCD)
#include <LiquidCrystal_I2C.h>
#endif
#if defined (USE_PARALELL_LCD)
#include <LiquidCrystal.h>
#endif


#if defined (USE_1602_LCD)
// definitions for a 1602 LCD
#define LCD_COLUMNS 16
#define LCD_ROWS 2
#endif
#if defined (USE_2004_LCD)
// definitions for a 2004 LCD
#define LCD_COLUMNS 20
#define LCD_ROWS 4
#endif

#define VERSION_EXAMPLE "1.3"

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

//#define F_INTERRUPTS                     20000 // Instead of default 15000 to support LEGO + RCMM protocols, but this in turn disables PENTAX and GREE protocols :-(

//#define IRMP_32_BIT                       1 // This enables MERLIN protocol, but decreases performance.

#include <irmpSelectAllProtocols.h>  // This enables all possible protocols

/*
 * After setting the modifiers we can include the code and compile it.
 */
#include <irmp.c.h>

IRMP_DATA irmp_data[1];

#if defined (USE_SERIAL_LCD)
LiquidCrystal_I2C myLCD(0x27, LCD_COLUMNS, LCD_ROWS);  // set the LCD address to 0x27 for a 20 chars and 2 line display
#endif
#if defined (USE_PARALELL_LCD)
LiquidCrystal myLCD(4, 5, 6, 7, 8, 9);
#endif

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

void handleReceivedIRData();
void irmp_result_print_LCD();

bool volatile sIRMPDataAvailable = false;

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

#if defined (USE_SERIAL_LCD)
    myLCD.init();
    myLCD.clear();
    myLCD.backlight();
#endif
#if defined (USE_PARALELL_LCD)
    myLCD.begin(LCD_COLUMNS, LCD_ROWS);
#endif
#if defined (USE_SERIAL_LCD) || defined (USE_PARALELL_LCD)
    myLCD.print(F("IRMP all V" VERSION_EXAMPLE));
    myLCD.setCursor(0, 1);
    myLCD.print(F(__DATE__));
#endif
}

void loop() {
    if (sIRMPDataAvailable) {
        sIRMPDataAvailable = false;

        /*
         * Serial output
         */
        irmp_result_print(&Serial, &irmp_data[0]);

#if defined (USE_SERIAL_LCD)
        irmp_disable_timer_interrupt(); // disable timer interrupt before sei() below, since it disturbs the serial output
#endif
        irmp_result_print_LCD();
#if defined (USE_SERIAL_LCD)
        irmp_enable_timer_interrupt();
#endif
    }
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
    sIRMPDataAvailable = true;
}

/*
 * LCD output for 1602 and 2004 LCDs
 */
void irmp_result_print_LCD() {
#if defined (USE_SERIAL_LCD) || defined (USE_PARALELL_LCD)
#  if (LCD_ROWS >= 4)
    const uint8_t tStartRow = 2;
    // clear data lines
    myLCD.setCursor(0, tStartRow);
    myLCD.print(F("                    "));
    myLCD.setCursor(0, tStartRow + 1);
    myLCD.print(F("                    "));

#  else
    const uint8_t tStartRow = 0;
    myLCD.clear();
#  endif

    // Show protocol name
    myLCD.setCursor(0, tStartRow);
    myLCD.print(F("P="));
#  if defined(__AVR__)
    const char* tProtocolStringPtr = (char*) pgm_read_word(&irmp_protocol_names[irmp_data[0].protocol]);
    myLCD.print((__FlashStringHelper *) (tProtocolStringPtr));
#  else
    myLCD.print(irmp_protocol_names[irmp_data[0].protocol]);
#  endif
    /*
     * Show address
     */
    myLCD.setCursor(0, tStartRow + 1);
    myLCD.print(F("A=0x"));
    myLCD.print(irmp_data[0].address, HEX);

    /*
     * Show command
     */
    uint16_t tCommand = irmp_data[0].command;
    myLCD.setCursor(9, tStartRow + 1);
#  if (LCD_COLUMNS <= 16)
    /*
     * render 16 bit commands
     */
    if (tCommand >= 0x100) {
        myLCD.print(F("0x"));
    } else {
        myLCD.print(F("C=0x"));
    }
#  else
    myLCD.print(F("C=0x"));
#  endif
    myLCD.print(tCommand, HEX);

    /*
     * Show repetition flag
     */
    if (irmp_data[0].flags & IRMP_FLAG_REPETITION) {
#  if (LCD_COLUMNS > 16)
        myLCD.setCursor(18, tStartRow + 1);
#  else
        myLCD.setCursor(15, tStartRow + 1);
#  endif
        myLCD.print('R');
    }

#endif // defined (USE_SERIAL_LCD) || defined (USE_PARALELL_LCD)
}

