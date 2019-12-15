/*
 *  Interrupt.cpp
 *
 *  Receives IR protocol data  by useing pin change interrupts and no polling by timer.
 *  This might be not working for all protocols.
 *  Tested for NEC, Kaseiko, Denon, RC6.
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
 *  To disable one of them or to enable other protocols, specify this before the "#include <irmp.h>" line.
 *  If you get warnings of redefining symbols, just ignore them or undefine them first (see Interrupt example).
 *  The exact names can be found in the library file irmpSelectAllProtocols.h (see Callback example).

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

#define VERSION_EXAMPLE "1.1"

/*
 * Set library modifiers first to set input pin etc.
 */
#if defined(ESP8266)
#define IRMP_INPUT_PIN 14 // D5
#elif defined(ESP32)
#define IRMP_INPUT_PIN 15
#elif defined(__STM32F1__)
#define IRMP_INPUT_PIN 3 // PA3
#else
#define IRMP_INPUT_PIN 3
// You can alternatively specify the input pin with port and bit number if you do not have the Arduino pin number at hand
//#define IRMP_PORT_LETTER D
//#define IRMP_BIT_NUMBER 3
#endif

#define IRMP_PROTOCOL_NAMES              1 // Enable protocol number mapping to protocol strings - needs some FLASH. Must before #include <irmp*>
#define IRMP_USE_COMPLETE_CALLBACK       1 // Enable callback functionality
#define IRMP_ENABLE_PIN_CHANGE_INTERRUPT 1 // Enable interrupt functionality

#if defined(__AVR__)
void initPCIInterrupt();
#else
#define IRMP_USE_ARDUINO_ATTACH_INTERRUPT
#endif

//#define SIZE_TEST
#ifdef SIZE_TEST
#define IRMP_SUPPORT_NEC_PROTOCOL        1
#else
#include <irmpSelectMain15Protocols.h>  // This enables 15 main protocols
#undef IRMP_SUPPORT_NEC42_PROTOCOL // this protocols is incompatible to NEC in interrupt mode, since it is the same as NEC but has longer data sections
#define IRMP_SUPPORT_NEC42_PROTOCOL      0
#endif

/*
 * After setting the modifiers we can include the code and compile it.
 */
#include <irmp.c.h>

IRMP_DATA irmp_data[1];

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
#ifdef IRMP_USE_ARDUINO_ATTACH_INTERRUPT
    attachInterrupt(digitalPinToInterrupt(IRMP_INPUT_PIN), irmp_PCI_ISR, CHANGE);
#else
    initPCIInterrupt();
#endif
    irmp_register_complete_callback_function(&handleReceivedIRData);

    Serial.println(F("Ready to receive IR signals at pin " STR(IRMP_INPUT_PIN)));
}

void loop() {
    /*
     * Put your code here
     */
}

/*
 * Here we know, that data is available.
 * Since this function is executed in Interrupt handler context, make it short and do not use delay() etc.
 * In order to enable other interrupts you can call sei() (enable interrupt again) after getting data.
 */
void handleReceivedIRData() {
    irmp_get_data(&irmp_data[0]);
    // enable interrupts
    interrupts(); // sei()
    irmp_result_print(&Serial, &irmp_data[0]);
}
