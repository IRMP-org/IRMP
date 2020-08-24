/*
 *  MinimalReceiver.cpp
 *
 *  Receives IR protocol data of 1 protocol.
 *  Example with minimal functionality to receive 1 protocol - especially suited for ATtinies.
 *
 *  *****************************************************************************************************************************
 *  To access the library files from your sketch, you have to first use `Sketch/Show Sketch Folder (Ctrl+K)` in the Arduino IDE.
 *  Then navigate to the parallel `libraries` folder and select the library you want to access.
 *  The library files itself are located in the `src` sub-directory.
 *  If you did not yet store the example as your own sketch, then with Ctrl+K you are instantly in the right library folder.
 *  *****************************************************************************************************************************
 *
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

/*
 * Set library modifiers first to set input pin etc.
 */
#include "PinDefinitionsAndMore.h"

#define IRMP_SUPPORT_NEC_PROTOCOL               1
/*
 * After setting the modifiers we can include the code and compile it.
 */
#include <irmp.c.h>

IRMP_DATA irmp_data;

void setup() {
    pinMode(LED_BUILTIN,OUTPUT);
    irmp_init();
}

void loop() {
    /*
     * Check if new data available and get them
     */
    if (irmp_get_data(&irmp_data)) {
        /*
         * Skip repetitions of command
         */
        if (!(irmp_data.flags & IRMP_FLAG_REPETITION)) {
            /*
             * Here data is available and is no repetition -> echo last bit of command at the internal led
             */
            digitalWrite(LED_BUILTIN, irmp_data.command & 0x01);
        }
    }
}
