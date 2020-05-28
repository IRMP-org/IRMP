/*
 *  Interrupt.cpp
 *
 *  Receives IR protocol data  by using pin change interrupts and no polling by timer.
 *  This might be not working for all protocols.
 *  Tested for NEC, Kaseiko, Denon, RC6, Samsung + Samsg32.
 *
 *  The following IR protocols are enabled:
 *      Sony SIRCS
 *      NEC + APPLE
 *      Samsung + Samsg32
 *      Kaseikyo
 *
 *      Plus 11 other main protocols
 *      JVC, NEC16, NEC42, Matsushita, DENON, Sharp, RC5, RC6 & RC6A, IR60 (SDA2008) Grundig, Siemens Gigaset, Nokia
 *
 *  To disable one of them or to enable other protocols, specify this before the "#include <irmp.c.h>" line.
 *  If you get warnings of redefining symbols, just ignore them or undefine them first (see Interrupt example).
 *  The exact names can be found in the library file irmpSelectAllProtocols.h (see Callback example).

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

/*
 * Set library modifiers first to set input pin etc.
 */
#include "PinDefinitionsAndMore.h"

#define IRMP_PROTOCOL_NAMES              1 // Enable protocol number mapping to protocol strings - needs some FLASH. Must before #include <irmp*>
#define IRMP_USE_COMPLETE_CALLBACK       1 // Enable callback functionality
#define IRMP_ENABLE_PIN_CHANGE_INTERRUPT 1 // Enable interrupt functionality

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

IRMP_DATA irmp_data;
void handleReceivedIRData();

void setup()
{
    Serial.begin(115200);
#if defined(__AVR_ATmega32U4__)
    while (!Serial); //delay for Leonardo, but this loops forever for Maple Serial
#endif
#if defined(SERIAL_USB)
    delay(2000); // To be able to connect Serial monitor after reset and before first printout
#endif
    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRMP));

    irmp_init();
    irmp_blink13(true); // Enable LED feedback
    irmp_register_complete_callback_function(&handleReceivedIRData);

#if defined(STM32F1xx)
    Serial.println(F("Ready to receive IR signals at pin PA4")); // the internal pin numbers are crazy for the STM32 Boards library
#else
    Serial.println(F("Ready to receive IR signals at pin " STR(IRMP_INPUT_PIN)));
#endif
}

void loop()
{
    /*
     * Put your code here
     */
}

/*
 * Here we know, that data is available.
 * Since this function is executed in Interrupt handler context, make it short and do not use delay() etc.
 * In order to enable other interrupts you can call sei() (enable interrupt again) after getting data.
 */
void handleReceivedIRData()
{
    irmp_get_data(&irmp_data);
    interrupts();
    // enable interrupts
    irmp_result_print(&irmp_data);
}
