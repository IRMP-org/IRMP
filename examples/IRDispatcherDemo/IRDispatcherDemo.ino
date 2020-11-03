/*
 *  IRDispatcherDemo.cpp
 *
 *  Receives NEC IR commands and maps them to different actions.
 *
 *  Copyright (C) 2020  Armin Joachimsmeyer
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
 * Set input pin and output pin definitions etc.
 */
#include "PinDefinitionsAndMore.h"
//#define IRMP_ENABLE_PIN_CHANGE_INTERRUPT   // Enable interrupt functionality - requires around 376 additional bytes of program space

#define IRMP_USE_COMPLETE_CALLBACK       1 // Enable callback functionality

#define IRMP_PROTOCOL_NAMES 1 // Enable protocol number mapping to protocol strings - requires some program space. Must before #include <irmp*>

#define IRMP_SUPPORT_NEC_PROTOCOL        1 // this enables only one protocol

#ifdef ALTERNATIVE_IRMP_FEEDBACK_LED_PIN
#define IRMP_FEEDBACK_LED_PIN   ALTERNATIVE_IRMP_FEEDBACK_LED_PIN
#endif
/*
 * After setting the definitions we can include the code and compile it.
 */
#include <irmp.c.h>

bool doBlink = false;
uint16_t sBlinkDelay = 200;

void doLedOn();
void doLedOff();
void doIncreaseBlinkFrequency();
void doDecreaseBlinkFrequency();
void doStop();
void doReset();
void doLedBlinkStart();
void doLedBlink20times();
void doTone1800();
void doTone2200();

/*
 * Set definitions and include IRCommandDispatcher library after the declaration of all commands to map
 */
#define INFO // to see some informative output
#define USE_IRMP_LIBRARY // must be specified before including IRCommandDispatcher.cpp.h to define which IR library to use
#include "IRCommandDispatcher.h" // Only for declarations, the library itself is included below after the definitions of the commands
#include "IRCommandMapping.h" // must be included before IRCommandDispatcher.cpp.h to define IR_ADDRESS and IRMapping and string "unknown".
#include "IRCommandDispatcher.cpp.h"

void handleReceivedIRData();

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
#if defined(__AVR_ATmega32U4__) || defined(SERIAL_USB) || defined(SERIAL_PORT_USBVIRTUAL)
    delay(2000); // To be able to connect Serial monitor after reset and before first printout
#endif
#if defined(ESP8266)
    Serial.println(); // to separate it from the internal boot output
#endif

    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRMP));

    // tone before setup, since it kills the IR timer settings
    tone(TONE_PIN, 1000);
    delay(50);
    noTone(TONE_PIN);

    IRDispatcher.init(); // This just calls irmp_init()
    irmp_register_complete_callback_function(&handleReceivedIRData); // cannot use IRDispatcher.loop as parameter for irmp_register_complete_callback_function
#ifdef ALTERNATIVE_IRMP_FEEDBACK_LED_PIN
    irmp_irsnd_LEDFeedback(true); // Enable receive signal feedback at ALTERNATIVE_IRMP_FEEDBACK_LED_PIN
#endif

    Serial.print(F("Ready to receive IR signals of protocols: "));
    irmp_print_active_protocols(&Serial);
#if defined(ARDUINO_ARCH_STM32)
    Serial.println(F("at pin " IRMP_INPUT_PIN_STRING));
#else
    Serial.println(F("at pin " STR(IRMP_INPUT_PIN)));
#endif

    Serial.print(F("Listening to commands of IR remote of type "));
    Serial.println(IR_REMOTE_NAME);

}

void loop()
{
    if (doBlink)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(sBlinkDelay);
        RETURN_IF_STOP; // if (IRDispatcher.requestToStopReceived) return;
        digitalWrite(LED_BUILTIN, LOW);
        delay(sBlinkDelay);
        RETURN_IF_STOP;
    }

    if (millis() - IRDispatcher.lastIRCodeMillis > 120000)
    {
        IRDispatcher.lastIRCodeMillis += 120000;
        /*
         * Short beep as remainder, if we did not receive any command in the last 10 seconds
         */
        doTone1800();
    }

    delay(10);
}

#if defined(ESP8266)
void ICACHE_RAM_ATTR handleReceivedIRData()
#elif defined(ESP32)
void IRAM_ATTR handleReceivedIRData()
#else
void handleReceivedIRData()
#endif
{
    interrupts(); // be careful with always executable commands which lasts longer than the IR repeat duration.
    // To enable delay() for commands
    IRDispatcher.loop(false); // cannot use IRDispatcher.loop as parameter for irmp_register_complete_callback_function
}

/*
 * Here the actions that are matched to IR keys
 */
void doLedOn()
{
    digitalWrite(LED_BUILTIN, HIGH);
    doBlink = false;
}
void doLedOff()
{
    digitalWrite(LED_BUILTIN, LOW);
    doBlink = false;
}
void doIncreaseBlinkFrequency()
{
    if (sBlinkDelay > 5)
    {
        sBlinkDelay -= sBlinkDelay / 4;
    }
}
void doDecreaseBlinkFrequency()
{
    sBlinkDelay += sBlinkDelay / 4;
}
void doStop()
{
    doBlink = false;
}
void doReset()
{
    doBlink = false;
    sBlinkDelay = 200;
    digitalWrite(LED_BUILTIN, LOW);
}
void doLedBlinkStart()
{
    doBlink = true;
}
void doLedBlink20times()
{
    doBlink = true;
    for (int i = 0; i < 20; ++i)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        RETURN_IF_STOP; // if (IRDispatcher.requestToStopReceived) return;
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
        RETURN_IF_STOP;
    }
    doBlink = false;
}
void doTone1800()
{
#ifdef IRMP_ENABLE_PIN_CHANGE_INTERRUPT
    tone(TONE_PIN, 1800, 200);
#else
    // You may use irmp_tone() instead
    tone(TONE_PIN, 1800);
    delay(200);
    noTone(TONE_PIN);
    irmp_init(); // restore timer for IR receive after using of tone
#endif
}
void doTone2200()
{
    // use IRMP compatible function for tone()
    irmp_tone(TONE_PIN, 2200, 50);
}
