/*
 * IRCommandDispatcher.cpp.h
 *
 * Library to process IR commands by calling functions specified in a mapping array.
 *
 * To run this example need to install the "IRLremote" and "PinChangeInterrupt" libraries under "Tools -> Manage Libraries..." or "Ctrl+Shift+I"
 * Use "IRLremote" and "PinChangeInterrupt" as filter string.
 *
 *  Copyright (C) 2019-2020  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of ServoEasing https://github.com/ArminJo/ServoEasing.
 *  This file is part of IRMP https://github.com/ukw100/IRMP.
 *
 *  ServoEasing is free software: you can redistribute it and/or modify
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
 */

#include <Arduino.h>

#include "IRCommandDispatcher.h"

//#define INFO // comment this out to see serial info output
//#define DEBUG // comment this out to see serial info output
#ifdef INFO
#  ifndef DEBUG
#define DEBUG
#  endif
#endif

IRCommandDispatcher IRDispatcher;

#if defined(USE_IRMP_LIBRARY)
#define getIRData() irmp_get_data((IRMP_DATA*)&IRReceivedData)

void IRCommandDispatcher::init() {
    irmp_init();
}


#elif defined(USE_IRL_REMOTE_LIBRARY)

#if (IR_RECEIVER_PIN != 2) && (IR_RECEIVER_PIN != 3)
#include <PinChangeInterrupt.h> // must be included before IRLremote.h if we do not use pin 2 or 3
#endif
/*
 * Only NEC protocol
 */
#include <IRLremote.h>      // include IR Remote library

// repeat definitions for NEC
#define IR_NEC_REPEAT_ADDRESS 0xFFFF
#define IR_NEC_REPEAT_CODE 0x0

CNec IRLremote;

/*
 * @return true if command or repeat was received
 */
bool getIRData() {
    static uint8_t sLastIRValue = COMMAND_EMPTY; // for repeat detection

    if (IRLremote.available()) {
        // Get the new data from the remote
        auto tIRData = IRLremote.read();
        IRDispatcher.IRReceivedData.address = tIRData.address;
        IRDispatcher.IRReceivedData.command = tIRData.command;
        if ((tIRData.address == IR_NEC_REPEAT_ADDRESS && tIRData.command == IR_NEC_REPEAT_CODE) || (sLastIRValue == tIRData.command)) {
            IRDispatcher.IRReceivedData.isRepeat = true;
            if (sLastIRValue != COMMAND_EMPTY) {
                IRDispatcher.IRReceivedData.command = sLastIRValue;
            }
        } else {
            sLastIRValue = tIRData.command;
            IRDispatcher.IRReceivedData.isRepeat = false;
        }
        return true;
    }
    return false;
}

void IRCommandDispatcher::init() {
    // Start reading the remote. PinInterrupt or PinChangeInterrupt* will automatically be selected
    if (!IRLremote.begin(IR_RECEIVER_PIN)) {
#ifdef INFO
        Serial.println(F("You did not choose a valid pin"));
#endif
    }
}
#endif

/*
 * Resets stop flag, gets new command, checks and runs it.
 *
 * @param aRunRejectedCommand if true run a command formerly rejected because of recursive calling.
 */
void IRCommandDispatcher::loop(bool aRunRejectedCommand) {
    /*
     * search IR code or take last rejected command and call associated function
     */
    if (aRunRejectedCommand && (rejectedRegularCommand != COMMAND_INVALID)) {
#ifdef INFO
        Serial.print(F("Take rejected command = 0x"));
        Serial.println(rejectedRegularCommand, HEX);
#endif
        IRReceivedData.command = rejectedRegularCommand;
        rejectedRegularCommand = COMMAND_INVALID;
        IRReceivedData.isRepeat = false;
        checkAndCallCommand();
    }

    if (getIRCommand(false)) {
        checkAndCallCommand();
    }
}

/*
 * Waits for next IR command, and returns IR code.
 * @return true if new command was received
 */
bool IRCommandDispatcher::getIRCommand(bool doWait) {
    do {
        if (getIRData()) {
            lastIRCodeMillis = millis();
#ifdef INFO
# if defined(USE_IRMP_LIBRARY)
            irmp_result_print((IRMP_DATA*)&IRReceivedData);
#  else
            Serial.print(F("A=0x"));
            Serial.print(IRReceivedData.address, HEX);
            Serial.print(F(" C=0x"));
            Serial.print(IRReceivedData.command, HEX);
            if ((IRReceivedData.isRepeat)) {
                Serial.print(F("R"));
            }
            Serial.println();
#  endif
#endif
            if (IRReceivedData.isRepeat || IRReceivedData.address == IR_ADDRESS) {
                // Received new code with right address or repeat
                return true;
            } else {
#ifdef INFO
                Serial.print(F(" Wrong address. Expected 0x"));
                Serial.println(IR_ADDRESS, HEX);
#endif
            }
        }
    } while (doWait);

    return false;
}

/*
 * Sets flags justCalledRegularIRCommand, executingRegularCommand
 */
uint8_t IRCommandDispatcher::checkAndCallCommand() {
    if (IRReceivedData.command == COMMAND_EMPTY) {
        return IR_CODE_EMPTY;
    }

    for (uint8_t i = 0; i < sizeof(IRMapping) / sizeof(struct IRToCommandMapping); ++i) {
        if (IRReceivedData.command == IRMapping[i].IRCode) {

#ifdef INFO
            const __FlashStringHelper * tCommandName = reinterpret_cast<const __FlashStringHelper *>(IRMapping[i].CommandString);
#endif
            /*
             * Check for repeat and if it is allowed for the current command
             */
            if (IRReceivedData.isRepeat && !(IRMapping[i].Flags & IR_COMMAND_FLAG_REPEATABLE)) {
#ifdef DEBUG
                Serial.print(F("Repeats of command \""));
                Serial.print(tCommandName);
                Serial.println("\" not accepted");
#endif
                return FOUND_BUT_REPEAT_NOT_ACCEPTED;
            }

            /*
             * Do not accept recursive call of the same command
             */
            if (currentRegularCommandCalled == IRReceivedData.command) {
#ifdef DEBUG
                Serial.print(F("Recursive command \""));
                Serial.print(tCommandName);
                Serial.println("\" not accepted");
#endif
                return FOUND_BUT_REPEAT_NOT_ACCEPTED;
            }

            /*
             * Handle stop command and requestToStopReceived flag
             */
            if (IRMapping[i].Flags & IR_COMMAND_FLAG_IS_STOP_COMMAND) {
                requestToStopReceived = true;
#ifdef INFO
                Serial.println(F("Stop command received"));
#endif
            } else {
                // lets start a new turn
                requestToStopReceived = false;
            }

#ifdef INFO
            Serial.print(F("Found command: "));
            Serial.println(tCommandName);
#endif

            bool tIsRegularCommand = !(IRMapping[i].Flags & IR_COMMAND_FLAG_EXECUTE_ALWAYS);
            if (tIsRegularCommand) {
                if (executingRegularCommand) {
                    /*
                     * A regular command may not be called as long as another regular command is running.
                     */
                    rejectedRegularCommand = IRReceivedData.command;
#ifdef INFO
                    Serial.println(F("Regular command rejected, since another regular command is already running"));
#endif
                    return FOUND_BUT_RECURSIVE_LOCK;
                }

                justCalledRegularIRCommand = true;
                executingRegularCommand = true; // set lock for recursive calls
                currentRegularCommandCalled = IRReceivedData.command;
                /*
                 * This call may be blocking!!!
                 */
                IRMapping[i].CommandToCall();
                executingRegularCommand = false;
                currentRegularCommandCalled = COMMAND_INVALID;
            } else {
                // short command here, just call
                IRMapping[i].CommandToCall();
            }
            return CALLED;
        }
    }
    return IR_CODE_NOT_FOUND;
}

void IRCommandDispatcher::setRequestToStopReceived() {
    requestToStopReceived = true;
}

/*
 * @return  true (and sets requestToStopReceived) if invalid or recursive regular IR command received
 */
bool IRCommandDispatcher::checkIRInputForAlwaysExecutableCommand() {
    uint8_t tCheckResult;
    if (getIRCommand(false)) {
        tCheckResult = checkAndCallCommand();
        if ((tCheckResult == IR_CODE_NOT_FOUND) || (tCheckResult == FOUND_BUT_RECURSIVE_LOCK)) {
            // IR command not found in mapping or received a recursive (while just running another one) regular command -> request stop
#ifdef INFO
            Serial.println(F("Invalid or recursive regular command received -> set stop"));
#endif
            requestToStopReceived = true; // return to loop
            return true;
        }
    }
    return false;
}

/*
 * Special delay function for the IRCommandDispatcher.
 * @return  true - if stop received
 */
bool IRCommandDispatcher::delayAndCheckForIRCommand(uint16_t aDelayMillis) {
    uint32_t tStartMillis = millis();

    do {
        if (IRDispatcher.checkIRInputForAlwaysExecutableCommand()) {
            return true;
        }

    } while (millis() - tStartMillis < aDelayMillis);
    return false;
}

void IRCommandDispatcher::printIRCommandString() {
#ifdef INFO
    Serial.print(F("IRCommand="));
    for (uint8_t i = 0; i < sizeof(IRMapping) / sizeof(struct IRToCommandMapping); ++i) {
        if (IRReceivedData.command == IRMapping[i].IRCode) {
            Serial.println(reinterpret_cast<const __FlashStringHelper *>(IRMapping[i].CommandString));
            return;
        }
    }
    Serial.println(reinterpret_cast<const __FlashStringHelper *>(unknown));
#endif
}

