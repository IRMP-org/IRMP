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
 *  Copyright (C) 2019-2022  Armin Joachimsmeyer
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

#include <Arduino.h>

#include "PinDefinitionsAndMore.h"

/*
 * Set input pin and output pin definitions etc.
 */
#define IRMP_PROTOCOL_NAMES              1 // Enable protocol number mapping to protocol strings - needs some program memory ~ 420 bytes here
#define IRMP_USE_COMPLETE_CALLBACK       1 // Enable callback functionality
//#define NO_LED_FEEDBACK_CODE   // Activate this if you want to suppress LED feedback or if you do not have a LED. This saves 14 bytes code and 2 clock cycles per interrupt.

#if __SIZEOF_INT__ == 4
#define F_INTERRUPTS                     20000 // Instead of default 15000 to support LEGO + RCMM protocols
#else
//#define F_INTERRUPTS                     20000 // Instead of default 15000 to support LEGO + RCMM protocols, but this in turn disables PENTAX and GREE protocols :-(
//#define IRMP_32_BIT                       1 // This enables MERLIN protocol, but decreases performance for AVR.
#endif

#include <irmpSelectAllProtocols.h>  // This enables all possible protocols
//#define IRMP_SUPPORT_SIEMENS_PROTOCOL 1

/*
 * After setting the definitions we can include the code and compile it.
 */
#include <irmp.hpp>

IRMP_DATA irmp_data;

/*
 * Activate the type of LCD you use
 * Default is parallel LCD with 2 rows of 16 characters (1602).
 * Serial LCD has the disadvantage, that the first repeat is not detected,
 * because of the long lasting serial communication.
 */
//#define USE_NO_LCD
//#define USE_SERIAL_LCD
/*
 * Define the size of your LCD
 */
//#define USE_2004_LCD
#if defined(USE_2004_LCD)
// definitions for a 2004 LCD
#define LCD_COLUMNS 20
#define LCD_ROWS 4
#else
#define USE_1602_LCD
// definitions for a 1602 LCD
#define LCD_COLUMNS 16
#define LCD_ROWS 2
#endif

#if defined(USE_SERIAL_LCD)
#include <LiquidCrystal_I2C.h> // Use an up to date library version, which has the init method
LiquidCrystal_I2C myLCD(0x27, LCD_COLUMNS, LCD_ROWS);  // set the LCD address to 0x27 for a 16 chars and 2 line display

#elif !defined(USE_NO_LCD)
#include <LiquidCrystal.h>
#define USE_PARALLEL_LCD
//LiquidCrystal myLCD(4, 5, 6, 7, 8, 9);
LiquidCrystal myLCD(7, 8, 3, 4, 5, 6);
#endif

#if defined(USE_SERIAL_LCD) || defined(USE_PARALLEL_LCD)
#define USE_LCD
#  if defined(ADC_UTILS_ARE_AVAILABLE)
// For cyclically display of VCC
#include "ADCUtils.hpp"
#define MILLIS_BETWEEN_VOLTAGE_PRINT 5000
uint32_t volatile sMillisOfLastVoltagePrint;
#  endif

void printIRResultOnLCD();
size_t printHex(uint16_t aHexByteValue);
#endif

void handleReceivedIRData();

bool volatile sIRMPDataAvailable = false;

void setup()
{
    Serial.begin(115200);
#if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/|| defined(SERIALUSB_PID) || defined(ARDUINO_attiny3217) \
    || defined(__AVR_ATtiny1616__)  || defined(__AVR_ATtiny3216__) || defined(__AVR_ATtiny3217__)
    delay(4000); // To be able to connect Serial monitor after reset or power on and before first printout
#endif
    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRMP));

    irmp_init();
    irmp_irsnd_LEDFeedback(true); // Enable receive signal feedback at LED_BUILTIN
    irmp_register_complete_callback_function(&handleReceivedIRData);

    Serial.print(F("Ready to receive IR signals of protocols: "));
    irmp_print_active_protocols(&Serial);
    Serial.println(F("at pin " STR(IRMP_INPUT_PIN)));
#if defined(USE_SERIAL_LCD)
    Serial.println(F("With serial LCD connection, the first repeat is not detected, because of the long lasting serial communication!"));
#endif

#if defined(USE_LCD) && defined(ADC_UTILS_ARE_AVAILABLE)
    getVCCVoltageMillivoltSimple(); // to initialize ADC mux and reference
#endif

#if defined(USE_SERIAL_LCD)
    myLCD.init();
    myLCD.clear();
    myLCD.backlight();
#endif
#if defined(USE_PARALLEL_LCD)
    myLCD.begin(LCD_COLUMNS, LCD_ROWS); // This also clears display
#endif

#if defined(USE_LCD)
    myLCD.setCursor(0, 0);
    myLCD.print(F("IRMP all  v" VERSION_IRMP));
    myLCD.setCursor(0, 1);
    myLCD.print(F(__DATE__));
#endif
}

void loop()
{
    if (sIRMPDataAvailable)
    {
        sIRMPDataAvailable = false;

        /*
         * Serial output
         * takes 2 milliseconds at 115200
         */
        irmp_result_print(&irmp_data);

#if defined(USE_LCD)
#  if defined(USE_SERIAL_LCD)
        // This suppresses the receive of the 1. NEC repeat
        disableIRTimerInterrupt(); // disable timer interrupt, since it disturbs the LCD serial output
#  endif
        printIRResultOnLCD();
#  if defined(USE_SERIAL_LCD)
        enableIRTimerInterrupt();
#  endif
#endif
    }

#if defined(USE_LCD) && defined(ADC_UTILS_ARE_AVAILABLE)
    /*
     * Periodically print VCC
     */
    if (millis() - sMillisOfLastVoltagePrint > MILLIS_BETWEEN_VOLTAGE_PRINT)
    {
        sMillisOfLastVoltagePrint = millis();
        uint16_t tVCC = getVCCVoltageMillivoltSimple();

        char tVoltageString[5];
        dtostrf(tVCC / 1000.0, 4, 2, tVoltageString);
        myLCD.setCursor(11, 0);
        myLCD.print(tVoltageString);
        myLCD.print('V');
    }
#endif
}

/*
 * Here we know, that data is available.
 * Since this function is executed in Interrupt handler context, make it short and do not use delay() etc.
 * In order to enable other interrupts you can call interrupts() (enable interrupt again) after getting data.
 */
#if defined(ESP8266) || defined(ESP32)
void IRAM_ATTR handleReceivedIRData()
#else
void handleReceivedIRData()
#endif
{

#if defined(USE_LCD) && defined(ADC_UTILS_ARE_AVAILABLE)
    // reset voltage display timer
    sMillisOfLastVoltagePrint = millis();
#endif

    /*
     * Just print the data to Serial and LCD
     */
    irmp_get_data(&irmp_data);
    sIRMPDataAvailable = true;
}

#if defined(USE_LCD)
/*
 * LCD output for 1602 and 2004 LCDs
 * 40 - 55 Milliseconds per initial output for a 1602 LCD
 * for a 2004 LCD the initial clearing adds 55 ms.
 * The expander runs at 100 kHz :-(
 * 8 milliseconds for 8 bit; 10 ms for 16 bit code output
 * 3 milliseconds for repeat output
 *
 */
void printIRResultOnLCD()
{
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
    if (sLastProtocolIndex != irmp_data.protocol || sLastProtocolAddress != irmp_data.address)
    {
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
#  if defined(__AVR__)
        const char *tProtocolStringPtr = (char*) pgm_read_word(&irmp_protocol_names[irmp_data.protocol]);
        myLCD.print((__FlashStringHelper*) (tProtocolStringPtr));
#  else
        myLCD.print(irmp_protocol_names[irmp_data.protocol]);
#  endif

        /*
         * Show address
         */
        myLCD.setCursor(0, tStartRow + 1);
        myLCD.print(F("A="));
        printHex(irmp_data.address);

#  if (LCD_COLUMNS > 16)
        /*
         * Print prefix of command here, since it is constant string
         */
        myLCD.setCursor(9, tStartRow + 1);
        myLCD.print(F("C="));
#  endif
    }
    else
    {
        /*
         * Show or clear repetition flag
         */
#  if (LCD_COLUMNS > 16)
        myLCD.setCursor(18, tStartRow + 1);
#  else
        myLCD.setCursor(15, tStartRow + 1);
#  endif
        if (irmp_data.flags & IRMP_FLAG_REPETITION)
        {
            myLCD.print('R');
            return; // Since it is a repetition, printed data has not changed
        }
        else
        {
            myLCD.print(' ');
        }
    }

    /*
     * Command prefix
     */
    uint16_t tCommand = irmp_data.command;

#  if (LCD_COLUMNS <= 16)
    // check if prefix position must change
    if (tDisplayWasCleared || (sLastCommand > 0x100 && tCommand < 0x100) || (sLastCommand < 0x100 && tCommand > 0x100))
    {
        sLastCommand = tCommand;
        /*
         * Print prefix for 8/16 bit commands
         */
        if (tCommand >= 0x100)
        {
            sLastCommandPrintPosition = 9;
        }
        else
        {
            myLCD.setCursor(9, tStartRow + 1);
            myLCD.print(F("C="));
            sLastCommandPrintPosition = 11;
        }
    }
#  endif

    /*
     * Command data
     */
    myLCD.setCursor(sLastCommandPrintPosition, tStartRow + 1);
    printHex(tCommand);
}

size_t printHex(uint16_t aHexByteValue) {
    myLCD.print(F("0x"));
    size_t tPrintSize = 2;
    if (aHexByteValue < 0x10 || (aHexByteValue > 0x100 && aHexByteValue < 0x1000)) {
        myLCD.print('0'); // leading 0
        tPrintSize++;
    }
    return myLCD.print(aHexByteValue, HEX) + tPrintSize;
}
#endif // defined(USE_LCD)
