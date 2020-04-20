/*
 *  OneProtocol.cpp
 *
 *  Receives IR protocol data. Only one protocol is activated.
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

//#define IRMP_PROTOCOL_NAMES 1 // Enable protocol number mapping to protocol strings - requires some FLASH. Must before #include <irmp*>

//#define IRMP_SUPPORT_SIRCS_PROTOCOL      1
#define IRMP_SUPPORT_NEC_PROTOCOL        1
//#define IRMP_SUPPORT_SAMSUNG_PROTOCOL    1
//#define IRMP_SUPPORT_KASEIKYO_PROTOCOL   1

//#define IRMP_SUPPORT_JVC_PROTOCOL        1
//#define IRMP_SUPPORT_NEC16_PROTOCOL      1
//#define IRMP_SUPPORT_NEC42_PROTOCOL      1
//#define IRMP_SUPPORT_MATSUSHITA_PROTOCOL 1
//#define IRMP_SUPPORT_DENON_PROTOCOL      1
//#define IRMP_SUPPORT_RC5_PROTOCOL        1
//#define IRMP_SUPPORT_RC6_PROTOCOL        1
//#define IRMP_SUPPORT_IR61_PROTOCOL       1
//#define IRMP_SUPPORT_GRUNDIG_PROTOCOL    1
//#define IRMP_SUPPORT_SIEMENS_PROTOCOL    1
//#define IRMP_SUPPORT_NOKIA_PROTOCOL      1

// use 20000 for Support of LEGO and RCMM
//#define F_INTERRUPTS                            20000   // interrupts per second, 50us, min: 10000, max: 20000, typ: 15000
//#define IRMP_SUPPORT_LEGO_PROTOCOL              1       // LEGO Power RC        >= 20000                 ~150 bytes
//#define IRMP_SUPPORT_RCMM_PROTOCOL              1       // RCMM 12,24, or 32    >= 20000                 ~150 bytes

/*
 * Protocols which are not enabled in the irmpSelectAllProtocols.h for the AllProtocol example
 */
//#define IRMP_SUPPORT_FAN_PROTOCOL               1       // FAN (ventilator)     >= 10000                  ~50 bytes     conflicts with NUBERT
//#define IRMP_SUPPORT_ORTEK_PROTOCOL             1       // ORTEK (Hama)         >= 10000                 ~150 bytes     conflicts with FDC and NETBOX
//#define IRMP_SUPPORT_ROOMBA_PROTOCOL            1       // iRobot Roomba        >= 10000                 ~150 bytes     conflicts with RC6
//#define IRMP_SUPPORT_RUWIDO_PROTOCOL            1       // RUWIDO, T-Home       >= 15000                 ~550 bytes     conflicts with DENON
//#define IRMP_SUPPORT_S100_PROTOCOL              1       // S100                 >= 10000                 ~250 bytes     conflicts with RC5
//#define IRMP_SUPPORT_ACP24_PROTOCOL             1       // ACP24                >= 10000                 ~250 bytes     conflicts with DENON
//#define IRMP_SUPPORT_PANASONIC_PROTOCOL         1       // PANASONIC Beamer     >= 10000                 ~250 bytes     conflicts with KASEIKYO
//#define IRMP_SUPPORT_RCII_PROTOCOL              1       // RCII T+A             >= 15000                 ~250 bytes     conflicts with GRUNDIG and NOKIA
/*
 * More protocol definitions can be found in irmpSelectAllProtocols.h
 */

/*
 * After setting the modifiers we can include the code and compile it.
 */
#include <irmp.c.h>

IRMP_DATA irmp_data;

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

void setup()
{
// initialize the digital pin as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
#if defined(__AVR_ATmega32U4__)
    while (!Serial); //delay for Leonardo, but this loops forever for Maple Serial
#endif
#if defined(SERIAL_USB)
    delay(2000); // To be able to connect Serial monitor after reset and before first printout
#endif
    // Just to know which program is running on my Arduino
#if defined(__ESP8266__)
    Serial.println();
#endif
    Serial.println(F("START " __FILE__ "\r\nVersion " VERSION_EXAMPLE " from " __DATE__));

    irmp_init();
//    irmp_blink13(true); // Enable LED feedback - commented out, since we use built in LED in loop below

#if defined(STM32F1xx)
    Serial.println(F("Ready to receive IR signals at pin PA4")); // the internal pin numbers are crazy for the STM32 Boards library
#else
    Serial.println(F("Ready to receive IR signals at pin " STR(IRMP_INPUT_PIN)));
#endif
}

void loop()
{
    /*
     * Check if new data available and get them
     */
    if (irmp_get_data(&irmp_data))
    {
        /*
         * Here data is available -> evaluate IR command
         */
        switch (irmp_data.command)
        {
        case 0x48:
            digitalWrite(LED_BUILTIN, HIGH);
            break;
        case 0x0B:
            digitalWrite(LED_BUILTIN, LOW);
            break;
        default:
            break;
        }

        irmp_result_print(&irmp_data);
    }
}
