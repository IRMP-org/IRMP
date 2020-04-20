/*
 *  AllProtocols.cpp
 *
 *  Accepts 40 protocols concurrently
 *  If you specify F_INTERRUPTS to 20000 at line 86 (default is 15000) it supports LEGO + RCMM protocols, but disables PENTAX and GREE protocols.
 *  if you see performance issues, you can disable MERLIN Protocol at line 88.
 *
 *  Uses a callback function which is called every time a complete IR command was received.
 *  Prints data to LCD connected parallel at pin 4-9 or serial at pin A4, A5
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

#define VERSION_EXAMPLE "1.3.1"

/*
 * Set library modifiers first to set input pin etc.
 */
#if defined(ESP8266)
#define BLINK_13_LED_IS_ACTIVE_LOW // The LED on my board is active LOW
#define IRMP_INPUT_PIN D5

#elif defined(ESP32)
#define IRMP_INPUT_PIN 15

#elif defined(STM32F1xx) || defined(__STM32F1__)
// BluePill in 2 flavors
// STM32F1xx is for "Generic STM32F1 series" from STM32 Boards from STM32 cores of Arduino Board manager
// __STM32F1__is for "Generic STM32F103C series" from STM32F1 Boards (STM32duino.com) of manual installed hardware folder
// Timer 3 of IRMP blocks PA6, PA7, PB0, PB1 for use by Servo or tone()
#define BLINK_13_LED_IS_ACTIVE_LOW // The LED on the BluePill is active LOW
#define IRMP_INPUT_PIN   PA6

#elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
#include "ATtinySerialOut.h"
#  if  defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#define IRMP_INPUT_PIN 0
#    if defined(ARDUINO_AVR_DIGISPARK)
#define LED_BUILTIN PB1
#    endif

#  else
#    if defined(ARDUINO_AVR_DIGISPARKPRO)
#define LED_BUILTIN 1 // On a Digispark Pro we have PB1 / D1 (Digispark library) or D9 (ATtinyCore lib) / on DigisparkBoard labeled as pin 1
#define IRMP_INPUT_PIN 9  // PA3 - on DigisparkBoard labeled as pin 9
#    else
#define IRMP_INPUT_PIN 3
#    endif
#  endif

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

IRMP_DATA irmp_data;

#if defined (USE_SERIAL_LCD) && defined (USE_PARALELL_LCD)
#error "Cannot use paralell and serial LCD simultaneously"
#endif

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
    Serial.begin(115200);
#if defined(__AVR_ATmega32U4__)
    while (!Serial); //delay for Leonardo, but this loops forever for Maple Serial
#endif
#if defined(SERIAL_USB)
    delay(2000); // To be able to connect Serial monitor after reset and before first printout
#endif
    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ "\r\nVersion " VERSION_EXAMPLE " from " __DATE__));

    irmp_init();
    irmp_blink13(true); // Enable LED feedback
    irmp_register_complete_callback_function(&handleReceivedIRData);

#if defined(STM32F1xx)
    Serial.println(F("Ready to receive IR signals at pin PA4")); // the internal pin numbers are crazy for the STM32 Boards library
#else
    Serial.println(F("Ready to receive IR signals at pin " STR(IRMP_INPUT_PIN)));
#endif

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
         * takes 2 milliseconds at 115200
         */
        irmp_result_print(&irmp_data);

#if defined (USE_SERIAL_LCD)
        disableIRTimerInterrupt(); // disable timer interrupt before sei() below, since it disturbs the serial output
#endif
        irmp_result_print_LCD();
#if defined (USE_SERIAL_LCD)
        enableIRTimerInterrupt();
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
    irmp_get_data(&irmp_data);
    sIRMPDataAvailable = true;
}

/*
 * LCD output for 1602 and 2004 LCDs
 * 40 - 55 Milliseconds per initial output for a 1602 LCD
 * for a 2014 LCD the initial clearing adds 55 ms.
 * The expander runs at 100 kHz :-(
 * 8 milliseconds for 8 bit; 10 ms for 16 bit code output
 * 3 milliseconds for repeat output
 *
 */
void irmp_result_print_LCD() {
#if defined (USE_SERIAL_LCD) || defined (USE_PARALELL_LCD)
    static uint8_t sLastProtocolIndex;
    static uint16_t sLastProtocolAddress;

#  if (LCD_ROWS >= 4)
    static uint8_t sLastCommandPrintPosition = 13;

    const uint8_t tStartRow = 2;

#  else
    static uint16_t sLastCommand;
    static uint8_t sLastCommandPrintPosition;

    const uint8_t tStartRow = 0;
    bool tDisplayWasCleared = false;
#  endif

    /*
     * Print only if protocol or address has changed
     */
    if (sLastProtocolIndex != irmp_data.protocol || sLastProtocolAddress != irmp_data.address) {
        sLastProtocolIndex = irmp_data.protocol;
        sLastProtocolAddress = irmp_data.address;
#  if (LCD_ROWS >= 4)
        // clear data lines
        myLCD.setCursor(0, tStartRow);
        myLCD.print(F("                    "));
        myLCD.setCursor(0, tStartRow + 1);
        myLCD.print(F("                    "));
#  else
        myLCD.clear();
        tDisplayWasCleared = true;
#  endif

        /*
         * Show protocol name
         */
        myLCD.setCursor(0, tStartRow);
        myLCD.print(F("P="));
#  if defined(__AVR__)
        const char* tProtocolStringPtr = (char*) pgm_read_word(&irmp_protocol_names[irmp_data.protocol]);
        myLCD.print((__FlashStringHelper *) (tProtocolStringPtr));
#  else
        myLCD.print(irmp_protocol_names[irmp_data.protocol]);
#  endif

        /*
         * Show address
         */
        myLCD.setCursor(0, tStartRow + 1);
        myLCD.print(F("A=0x"));
        myLCD.print(irmp_data.address, HEX);

#  if (LCD_COLUMNS > 16)
        /*
         * Print prefix of command here, since it is constant string
         */
        myLCD.setCursor(9, tStartRow + 1);
        myLCD.print(F("C=0x"));
#  endif
    } else {
        /*
         * Show or clear repetition flag
         */
#  if (LCD_COLUMNS > 16)
        myLCD.setCursor(18, tStartRow + 1);
#  else
        myLCD.setCursor(15, tStartRow + 1);
#  endif
        if (irmp_data.flags & IRMP_FLAG_REPETITION) {
            myLCD.print('R');
            return; // Since it is a repetition, printed data has not changed
        } else {
            myLCD.print(' ');
        }
    }

    /*
     * Command prefix
     */
    uint16_t tCommand = irmp_data.command;

#  if (LCD_COLUMNS <= 16)
    // check if prefix will change
    if (tDisplayWasCleared || (sLastCommand > 0x100 && tCommand < 0x100) || (sLastCommand < 0x100 && tCommand > 0x100)) {
        sLastCommand = tCommand;
        /*
         * Print prefix of command
         */
        myLCD.setCursor(9, tStartRow + 1);

        /*
         * Print prefix for 8/16 bit commands
         */
        if (tCommand >= 0x100) {
            myLCD.print(F("0x"));
            sLastCommandPrintPosition = 11;
        } else {
            myLCD.print(F("C=0x"));
            sLastCommandPrintPosition = 13;
        }
    }
#  endif

    /*
     * Command data
     */
    myLCD.setCursor(sLastCommandPrintPosition, tStartRow + 1);
    if (irmp_data.command < 0x10) {
        // leading 0
        myLCD.print('0');
    }
    myLCD.print(tCommand, HEX);

#endif // defined (USE_SERIAL_LCD) || defined (USE_PARALELL_LCD)
}

