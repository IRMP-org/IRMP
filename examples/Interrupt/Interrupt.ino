/*
 *  Interrupt.cpp
 *
 *  Receives IR protocol data by using pin change interrupts and no polling by timer.
 *  !!! This WILL NOT work for all protocols.!!!
 *  Tested for NEC, Kaseiko, Denon, RC6, Samsung + Samsg32.
 *
 *  To disable one of them or to enable other protocols, specify this before the "#include <irmp.hpp>" line.
 *  If you get warnings of redefining symbols, just ignore them or undefine them first (see Interrupt example).
 *  The exact names can be found in the library file irmpSelectAllProtocols.h (see Callback example).
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

/*
 * Set input pin and output pin definitions etc.
 */
#include "PinDefinitionsAndMore.h"

#define IRMP_PROTOCOL_NAMES              1 // Enable protocol number mapping to protocol strings - needs some FLASH. Must before #include <irmp*>
#define IRMP_USE_COMPLETE_CALLBACK       1 // Enable callback functionality
#define IRMP_ENABLE_PIN_CHANGE_INTERRUPT   // Enable interrupt functionality

#define IRMP_SUPPORT_NEC_PROTOCOL               1       // NEC + APPLE + ONKYO  >= 10000                 ~300 bytes
#define IRMP_SUPPORT_SAMSUNG_PROTOCOL           1       // Samsung + Samsg32    >= 10000                 ~300 bytes
#define IRMP_SUPPORT_KASEIKYO_PROTOCOL          1       // Kaseikyo             >= 10000                 ~250 bytes
#define IRMP_SUPPORT_RC6_PROTOCOL               1       // RC6 & RC6A           >= 10000                 ~250 bytes
#define IRMP_SUPPORT_DENON_PROTOCOL             1       // DENON, Sharp         >= 10000                 ~250 bytes

/*
 * After setting the definitions we can include the code and compile it.
 */
#include <irmp.hpp>

IRMP_DATA irmp_data;
#define PROCESS_IR_RESULT_IN_MAIN_LOOP
#if defined(PROCESS_IR_RESULT_IN_MAIN_LOOP) || defined(ARDUINO_ARCH_MBED) || defined(ESP32)
volatile bool sIRDataJustReceived = false;
#endif

void handleReceivedIRData();
void evaluateIRCommand(uint16_t aAddress, uint16_t aCommand, uint8_t aFlags);

void setup() {
    Serial.begin(115200);
#if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/ \
    || defined(SERIALUSB_PID)  || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_attiny3217)
    delay(4000); // To be able to connect Serial monitor after reset or power up and before first printout
#endif
    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRMP));

    irmp_init();
    irmp_irsnd_LEDFeedback(true); // Enable receive signal feedback at LED_BUILTIN
    irmp_register_complete_callback_function(&handleReceivedIRData);

    Serial.print(F("Ready to receive IR signals of protocols: "));
    irmp_print_active_protocols(&Serial);
    Serial.println(F("at pin " STR(IRMP_INPUT_PIN)));
}

void loop() {
#if defined(PROCESS_IR_RESULT_IN_MAIN_LOOP) || defined(ARDUINO_ARCH_MBED) || defined(ESP32)
    if (sIRDataJustReceived) {
        sIRDataJustReceived = false;
        evaluateIRCommand(irmp_data.address, irmp_data.command, irmp_data.flags);
        irmp_result_print(&irmp_data); // this is not allowed in ISR context for any kind of RTOS
    }
#endif
    /*
     * Put your code here
     */
}

/*
 * Callback function
 * Here we know, that data is available.
 * This function is executed in ISR (Interrupt Service Routine) context (interrupts are blocked here).
 * Make it short and fast and keep in mind, that you can not use delay(), prints longer than print buffer size etc.,
 * because they require interrupts enabled to return.
 * In order to enable other interrupts you can call sei() (enable interrupt again) after evaluating/copying data.
 * Good practice, but somewhat more complex, is to copy relevant data and signal receiving to main loop.
 */
#if defined(ESP8266) || defined(ESP32)
void IRAM_ATTR handleReceivedIRData()
#else
void handleReceivedIRData()
#endif
{
    irmp_get_data(&irmp_data);
#if defined(PROCESS_IR_RESULT_IN_MAIN_LOOP) || defined(ARDUINO_ARCH_MBED) || defined(ESP32)
    /*
     * Set flag to trigger printing of results in main loop,
     * since printing should not be done in a callback function
     * running in ISR (Interrupt Service Routine) context where interrupts are disabled.
     */
    sIRDataJustReceived = true;
#else
    interrupts(); // enable interrupts
    evaluateIRCommand(irmp_data.address, irmp_data.command, irmp_data.flags);
    irmp_result_print(&irmp_data); // This is not recommended, but simpler and works, except for any kind of RTOS like on ESP and MBED.
#endif
}

void evaluateIRCommand(uint16_t aAddress, uint16_t aCommand, uint8_t aFlags)
{
    /*
     * Filter for commands from the WM010 IR Remote
     */
    if (aAddress == 0xF708)
    {
        /*
         * Skip repetitions of command
         */
        if (!(aAddress & IRMP_FLAG_REPETITION))
        {
            /*
             * Evaluation of IR command
             */
            switch (aAddress)
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
        }
    }
}
