/*
 *  ReceiveAndSend.cpp
 *
 *  Serves as a IR remote macro expander
 *  Receives Samsung32 protocol and  on receiving a specified input frame, it sends multiple Samsung32 frames.
 *  This works as a Netflix-key emulation for my oldSamsung TV.
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

#define VERSION_EXAMPLE "1.0"

/*
 * Set library modifiers first to set input and output pin etc.
 */
#if defined(ESP8266)
#define IRMP_INPUT_PIN 14   // D5
#define IRSND_OUTPUT_PIN 12 // D6
#define TONE_PIN 15         //D8

#define BLINK_13_LED_IS_ACTIVE_LOW // The LED on my board is active LOW

#elif defined(ESP32)
#define IRMP_INPUT_PIN 15   // D15
#define TONE_PIN 16

#elif defined(STM32F1xx) || defined(__STM32F1__)
// BluePill in 2 flavors
// STM32F1xx is for "Generic STM32F1 series" from STM32 Boards from STM32 cores of Arduino Board manager
// __STM32F1__is for "Generic STM32F103C series" from STM32F1 Boards (STM32duino.com) of manual installed hardware folder
#define IRMP_INPUT_PIN 4 // PA4
#define IRSND_OUTPUT_PIN 5 // PA5
#define TONE_PIN 6

#define BLINK_13_LED_IS_ACTIVE_LOW // The LED on the BluePill is active LOW

#elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
#include "ATtinySerialOut.h"
#include "ATtinyUtils.h" // for changeDigisparkClock() and definition of LED_BUILTIN
#  if  defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#define IRMP_INPUT_PIN 0
#define IRSND_OUTPUT_PIN 4 // Pin 1 is internal LED on Digispark board, Pin 2 is serial output with ATtinySerialOut, Pin3 is USB+
#define TONE_PIN 3
//#define IRMP_MEASURE_TIMING
//#define IRMP_TIMING_TEST_PIN 3
#  else
#define TONE_PIN 5
#    if defined(ARDUINO_AVR_DIGISPARKPRO)
#define IRMP_INPUT_PIN 9  // PA3 - on Digispark board labeled as pin 9
#define IRSND_OUTPUT_PIN 8  // PA2 - on Digispark board labeled as pin 8
#    else
#define IRMP_INPUT_PIN 3
#define IRSND_OUTPUT_PIN 2
#    endif
#  endif

#else
#define IRMP_INPUT_PIN 3
#define IRSND_OUTPUT_PIN 4
#define TONE_PIN 5
#endif

#define IRMP_PROTOCOL_NAMES 1 // Enable protocol number mapping to protocol strings - needs some FLASH. Must before #include <irmp*>

#define IRMP_SUPPORT_SAMSUNG_PROTOCOL     1
#define IRSND_SUPPORT_SAMSUNG_PROTOCOL    1

/*
 * After setting the modifiers we can include the code and compile it.
 */
#define USE_ONE_TIMER_FOR_IRMP_AND_IRSND // otherwise we get an error: redefinition of 'void __vector_8()
#include <irmp.c.h>
#include <irsnd.c.h>

IRMP_DATA irmp_data[1];
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
#if defined(ARDUINO_AVR_DIGISPARK) || defined(ARDUINO_AVR_DIGISPARKPRO)
    changeDigisparkClock();
#endif
    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ "\r\nVersion " VERSION_EXAMPLE " from " __DATE__));
    irmp_init();
    irmp_blink13(true); // Enable LED feedback for receive

    irsnd_init();
    irsnd_blink13(true); // Enable LED feedback for send

#if defined(STM32F1xx)
    Serial.println(F("Ready to receive IR signals at pin PA4")); // the internal pin numbers are crazy for the STM32 Boards library
    Serial.println(F("Ready to send IR signals at pin PA5")); // the internal pin numbers are crazy for the STM32 Boards library
#else
    Serial.println(F("Ready to receive IR signals at pin " STR(IRMP_INPUT_PIN)));
    Serial.println(F("Ready to send IR signals at pin " STR(IRSND_OUTPUT_PIN)));
#endif

    irsnd_data.protocol = IRMP_SAMSUNG32_PROTOCOL;
    irsnd_data.address = 0x0707;
    irsnd_data.flags = 1; // repeat frame 1 time

}

void loop()
{
    /*
     * Check if new data available and get them
     */
    if (irmp_get_data(&irmp_data[0]))
    {
        irmp_result_print(&irmp_data[0]);

        /*
         * Here data is available -> evaluate IR command
         */
        switch (irmp_data[0].command)
        {
        case 0xB847:
            /*
             * Do beep
             */
            Serial.println(F("Special key detected, now beep and send IR frames"));

            tone(TONE_PIN, 2200);
            delay(200);
            noTone(TONE_PIN);
            irmp_init(); // restore timer for IR receive after using of tone
            delay(1000);

            irsnd_data.command = 0xE51A; // MENU
            irsnd_send_data(&irsnd_data, true); // Wait for frame to end. This stores timer state and restores it after sending
            delay(500);

            irsnd_data.command = 0x9E61; // Down arrow
            irsnd_send_data(&irsnd_data, true);

            /*
             * Place your delays and codes here
             */
            break;
        default:
            break;
        }

    }
}
