/*
 *  OneProtocol.cpp
 *
 *  Receives IR protocol data. Only one protocol is activated.
 *
 *  Copyright (C) 2019-2020  Armin Joachimsmeyer
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

/*
 * Set input pin and output pin definitions etc.
 */
#include "PinDefinitionsAndMore.h" // Sets input pin to 3

//#define NO_LED_FEEDBACK_CODE   // Activate this if you want to suppress LED feedback or if you do not have a LED. This saves 14 bytes code and 2 clock cycles per interrupt.

#define IRMP_PROTOCOL_NAMES 1 // Enable protocol number mapping to protocol strings - requires some FLASH.

//#define IRMP_SUPPORT_SIRCS_PROTOCOL      1
#define IRMP_SUPPORT_NEC_PROTOCOL        1  // includes APPLE and ONKYO protocols
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

// use F_INTERRUPTS=20000 for Support of LEGO and RCMM
//#define F_INTERRUPTS                     20000   // interrupts per second, 50us, min: 10000, max: 20000, typ: 15000
//#define IRMP_SUPPORT_LEGO_PROTOCOL       1       // LEGO Power RC        >= 20000  ~150 bytes
//#define IRMP_SUPPORT_RCMM_PROTOCOL       1       // RCMM 12,24, or 32    >= 20000  ~150 bytes

/*
 * Protocols which are not enabled in the irmpSelectAllProtocols.h for the AllProtocol example
 */
//#define IRMP_SUPPORT_FAN_PROTOCOL        1       // FAN (ventilator)     >= 10000   ~50 bytes     conflicts with NUBERT
//#define IRMP_SUPPORT_ORTEK_PROTOCOL      1       // ORTEK (Hama)         >= 10000  ~150 bytes     conflicts with FDC and NETBOX
//#define IRMP_SUPPORT_ROOMBA_PROTOCOL     1       // iRobot Roomba        >= 10000  ~150 bytes     conflicts with RC6
//#define IRMP_SUPPORT_RUWIDO_PROTOCOL     1       // RUWIDO, T-Home       >= 15000  ~550 bytes     conflicts with DENON
//#define IRMP_SUPPORT_S100_PROTOCOL       1       // S100                 >= 10000  ~250 bytes     conflicts with RC5
//#define IRMP_SUPPORT_ACP24_PROTOCOL      1       // ACP24                >= 10000  ~250 bytes     conflicts with DENON
//#define IRMP_SUPPORT_PANASONIC_PROTOCOL  1       // PANASONIC Beamer     >= 10000  ~250 bytes     conflicts with KASEIKYO
//#define IRMP_SUPPORT_RCII_PROTOCOL       1       // RCII T+A             >= 15000  ~250 bytes     conflicts with GRUNDIG and NOKIA
/*
 * More protocol definitions can be found in irmpSelectAllProtocols.h
 */

/*
 * After setting the definitions we can include the code and compile it.
 */
#include <irmp.hpp>

IRMP_DATA irmp_data;

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
#if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/|| defined(SERIALUSB_PID) || defined(ARDUINO_attiny3217)
    delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#endif
    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRMP));

    irmp_init();

    Serial.print(F("Ready to receive IR signals of protocols: "));
    irmp_print_active_protocols(&Serial);
    Serial.println(F("at pin " STR(IRMP_INPUT_PIN)));

    irmp_irsnd_LEDFeedback(true); // Enable receive signal feedback at ALTERNATIVE_IR_FEEDBACK_LED_PIN
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
        case 0x40:
            Serial.println(F("Received right code an do something"));
            digitalWrite(LED_BUILTIN, HIGH);
            break;
        default:
            break;
        }

        irmp_result_print(&irmp_data);
    }
}
