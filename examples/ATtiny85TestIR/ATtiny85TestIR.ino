/*
 * Test and display infrared sequence codes on the ATTINY85 (Digispark) with IRMP library.
 *
 * This code allows debugging IR keys through an ATtiny85 microcontroller (e.g.,
 * Digispark Digistump ATTINY85).
 *
 * ATtiny85 presents itself to the PC as a keyboard USB HID; no PC driver is needed.
 * 
 * How to use:
 * - Uncomment "#define USE_KEYBOARD" if using the keyboard USB HID
 *   (otherwise only the internal LED is used)
 * - Copy Arduino\AtTinyTestIR\libraries\DKeyboard to Arduino\libraries\DKeyboard
 * - Connect the Attiny85 to the PC via USB (with IR receiver connected to pin IRMP_INPUT_PIN)
 * - Compile and load
 * - Open a Notepad, maximize it and put the Windows focus there
 * - Press a key on the IR remote
 * - Wait for the Digispark ATTINY85 led to switch on (means valid IR received)
 * - Wait for the led to switch off
 * - Read the IR code on the notepad window
 *
 * Notes:
 *  - This code is VERY slow and should be only used for detecting/debugging IR codes
 *  - DKeyboard library is needed (modified tiny DigisparkKeyboard library)
 *  - False errors of missing USB driver messages can occur with Windows (should be ignored)
 *  - If no data is shown, increase KEYBOARD_DELAY timer
 */

// Begin Configuration
//#define USE_KEYBOARD // Dump IR code via USB keyboard
#define LED_PIN 1 // = Digispark P1 (internal LED)
#define IRMP_INPUT_PIN 2 // = Digispark P2
// End Configuration

#include <irmpSelectMain15Protocols.h>  // 3 main protocols with Digiapark Attiny85
#include <irmp.c.h>

IRMP_DATA irmp_data[1];

#ifdef USE_KEYBOARD
#include <avr/interrupt.h>
#define KEYBOARD_DELAY 1500 // Time to wait for the PC keyboard driver to become ready
#include "DKeyboard.h" // Use a modified tiny DigisparkKeyboard library
DKeyboardDevice Db = DKeyboardDevice(); // instantiate the Keyboard driver
char buffer[100];
#endif // USE_KEYBOARD

//---------------------------------------------------------------------------------------------------------------------------------------------

void setup() {
# ifdef LED_PIN
    pinMode(LED_PIN, OUTPUT); //LED on Model A
# endif // LED_PIN
    irmp_init();
}

//---------------------------------------------------------------------------------------------------------------------------------------------

void loop() {
    if (irmp_get_data( & irmp_data[0])) {
        if (!(irmp_data[0].flags & IRMP_FLAG_REPETITION)) {
            /*
             * Here data is available and is no repetition -> evaluate IR command
             */
            // Pressure of the first key
#         ifdef LED_PIN
            digitalWrite(LED_PIN, HIGH);
#         endif // LED_PIN
#         ifdef USE_KEYBOARD
            Db.delay(500); // ensure that the transmission is performed before the interrupt is turned off
#         else
            delay(500);
#         endif // USE_KEYBOARD
#         ifdef LED_PIN
            digitalWrite(LED_PIN, LOW);
#         endif // LED_PIN
        }
#     ifdef USE_KEYBOARD
        disableIRTimerInterrupt();
        Db.sendKeyStroke(0);
        Db.delay(KEYBOARD_DELAY); // This is needed to wait for the PC keyboard driver to become ready
        Db.sendKeyStroke(0); // This will ensure no character will be lost
        Db.print("Protocol ");
        Db.println(irmp_data[0].protocol, HEX);
        Db.print("Address ");
        Db.println(irmp_data[0].address, HEX);
        Db.print("Key received ");
        Db.println(irmp_data[0].command, HEX);
        if (irmp_data[0].flags & IRMP_FLAG_REPETITION)
            Db.println("Repeat");
        Db.println("");
        enableIRTimerInterrupt();
#     endif // USE_KEYBOARD
    }

# ifdef USE_KEYBOARD
    Db.delay(300);
# else
    delay(500);
# endif // USE_KEYBOARD
}
