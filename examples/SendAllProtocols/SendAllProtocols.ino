/*
 *  SendAllProtocols.cpp
 *
 *  Sends 39 protocols for testing purposes
 *
 *  To disable one of them or to enable other protocols, specify this before the "#include <irmp.hpp>" line.
 *  If you get warnings of redefining symbols, just ignore them or undefine them first (see Interrupt example).
 *  The exact names can be found in the library file irmpSelectAllProtocols.h (see Callback example).
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
 * Set library modifiers first to set output pin etc.
 */
#include "PinDefinitionsAndMore.h"
#define IRSND_IR_FREQUENCY          38000

#if ! (defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__))
#define IRSND_PROTOCOL_NAMES        1
#endif
//#define IRSND_GENERATE_NO_SEND_RF // for back to back tests

#include <irsndSelectAllProtocols.h>
/*
 * After setting the definitions we can include the code and compile it.
 */
#include <irsnd.hpp>

IRMP_DATA irsnd_data;

void setup()
{
    Serial.begin(115200);
#if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/ \
    || defined(SERIALUSB_PID)  || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_attiny3217)
    delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#endif
#if defined(ESP8266)
    Serial.println(); // to separate it from the internal boot output
#endif

    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRMP));

    Serial.print(F("Send sample frequency="));
    Serial.print(F_INTERRUPTS);
    Serial.println(F(" Hz"));

    irsnd_init();
    irmp_irsnd_LEDFeedback(true); // Enable send signal feedback at LED_BUILTIN

    Serial.println(F("Send IR signals at pin " STR(IRSND_OUTPUT_PIN)));
    delay(1000);
}

void loop()
{
    static uint8_t sAddress = 1;
    static uint8_t sCommand = 1;
    static uint8_t sRepeats = 0;

    for (uint_fast8_t i = 0; i < sizeof(irsnd_used_protocol_index); ++i)
    {
        irsnd_data.protocol = pgm_read_byte(&irsnd_used_protocol_index[i]);
        irsnd_data.address = sAddress;
        irsnd_data.command = sCommand;
        irsnd_data.flags = sRepeats;

        // true = wait for frame and trailing space/gap to end. This stores timer state and restores it after sending.
        if (!irsnd_send_data(&irsnd_data, true))
        {
            Serial.println(F("Protocol not found")); // name of protocol is printed by irsnd_data_print()
        }

        irsnd_data_print(&Serial, &irsnd_data);

        sAddress++;
        sCommand++;
        delay(2000);
    }
    Serial.println();
    Serial.println();

    sRepeats++;

    // we have 0x27 protocols now start with next number range
    sAddress = (sAddress & 0xC0) + 0x40;
    sCommand = sAddress;
    Serial.print(F("Now sending all with number of repeats="));
    Serial.println(sRepeats);
}
