/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * irmpconfig.h
 *
 * DO NOT INCLUDE THIS FILE, WILL BE INCLUDED BY IRMP.H!
 *
 * Copyright (c) 2009-2019 Frank Meyer - frank(at)fli4l.de
 * Extensions for PIC 12F1820 W.Strobl 2014-07-20
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef IRMP_H
#  error please include only irmp.h, not irmpconfig.h
#endif

#ifdef IRMPCONFIG_STAGE1_H

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change IRMP_32_BIT here
 *
 * Normally, IRMP_32_BIT ist 0. Then we use irmp.command as a 16 bit value.
 * If you set IRMP_32_BIT to 1, we will use irmp.command as a 32 bit value.
 * A 32 bit command costs more CPU resources on 8 bit processors (e.g. AVR),
 * but there are IR protocols which need more than 16 bits for a reasonable
 * command value.
 *
 * If you want to use one of the following protocols, set IRMP_32_BIT to 1,
 * otherwise set it to 0:
 *    - MERLIN
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef IRMP_32_BIT
#  define IRMP_32_BIT     0                                                                               // use 32 bit command value, 0 or 1
#endif

#endif // IRMPCONFIG_STAGE1_H
#ifdef IRMPCONFIG_STAGE2_H

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change F_INTERRUPTS if you change the number of interrupts per second,
 * Normally, F_INTERRUPTS should be in the range from 10000 to 15000, typical is 15000
 * A value above 15000 costs additional program space, absolute maximum value is 20000.
 * A value of 20000 is needed for Support of LEGO and RCMM, but it prevents using PENTAX or GREE
 *  since for 20000 they have 8 bit overflow issues because of the long start bits.
 * On PIC with XC8/C18 Compiler, use 15151 as the correct value.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef F_INTERRUPTS
#  define F_INTERRUPTS                          15000   // interrupts per second, 66,66us, min: 10000, max: 20000, typ: 15000
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change hardware pin here for Arduino IDE if no IRMP_INPUT_PIN specified
 * Should be first, since it covers multiple platforms
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#if defined(ARDUINO)                                                   	// Arduino IDE for different platforms
#  if !defined (IRMP_INPUT_PIN)											// Arduino IDE uses IRMP_INPUT_PIN
#    define IRMP_INPUT_PIN 3
#  endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change hardware pin here for ATMEL ATMega/ATTiny/XMega
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined (ATMEL_AVR) || defined (__AVR_XMEGA__)                    // use PD4
#  ifndef IRMP_PORT_LETTER
#    define IRMP_PORT_LETTER                    D                       // Port D is Port for Arduino pin 0 to 7. B is for pin 8 to 13, C is for A0 to A5
#  endif
#  ifndef IRMP_BIT_NUMBER
#    define IRMP_BIT_NUMBER                     3
#  endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change hardware pin here for PIC C18 or XC8 compiler
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined (PIC_C18)                                                 // use RB4 as IR input on PIC (C18 or XC8 compiler)
#  if defined(__12F1840)
#    define IRMP_PIN                            RA5                     // on 12F1840 with XC8 compiler
#  else
#    define IRMP_PIN                            PORTBbits.RB4           // PIC C18
#  endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change hardware pin here for PIC CCS compiler
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined (PIC_CCS)
#  define IRMP_PIN                              PIN_B4                  // use PB4 as IR input on PIC (CCS compiler)

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change hardware pin here for ARM STM32
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined (ARM_STM32)                                               // use C13 as IR input on STM32
#  define IRMP_PORT_LETTER                      C
#  define IRMP_BIT_NUMBER                       13

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Hardware pin for ARM STM32 (HAL) - don't change here, define IRMP_RECEIVE_GPIO_Port & IRMP_RECEIVE_PIN in STM32Cube (Main.h)
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined (ARM_STM32_HAL)                                           // STM32: IRMP_RECEIVE_GPIO_Port & IRMP_RECEIVE_PIN must be defined in STM32Cube
#  define IRMP_PORT_LETTER                      IRMP_Receive_GPIO_Port
#  define IRMP_BIT_NUMBER                       IRMP_Receive_Pin

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change hardware pin here for Stellaris ARM Cortex M4
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined (STELLARIS_ARM_CORTEX_M4)                                 // use B4 as IR input on Stellaris LM4F
#  define IRMP_PORT_LETTER                      B
#  define IRMP_BIT_NUMBER                       4

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change hardware pin here for STM8
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined (SDCC_STM8)                                               // use PA1 as IR input on STM8
#  define IRMP_PORT_LETTER                      A
#  define IRMP_BIT_NUMBER                       1

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change hardware pin here for ESP8266
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined (__xtensa__)
#  define IRMP_BIT_NUMBER                       12                      // use GPIO12 (Pin 7 UEXT) on ESP8266-EVB evaluation board


/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change hardware pin here for Teensy 3.x with teensyduino gcc compiler
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined (TEENSY_ARM_CORTEX_M4)
#  define IRMP_PIN                              1                       // use Digital pin 1 as IR input on Teensy

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change hardware pin here for MBED
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined(__MBED__)
#  define IRMP_PIN                              P0_22                   // use P1_27 on LPC1347
#  define IRMP_PINMODE                          PullUp                  // hardware dependent

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change hardware pin here for ChibiOS HAL
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined(_CHIBIOS_HAL_)
#  define IRMP_PIN                              LINE_IR_IN              // use pin names as defined in the board config file, prefixed with "LINE_"
#  define IRMP_LOGGING_SD                       SD1                     // the ChibiOS HAL Serial Driver instance to log to
                                                                        // (when IRMP_LOGGING is enabled below).
                                                                        // Make sure SERIAL_BUFFERS_SIZE is large enough when enabling logging

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Handling of unknown target system: DON'T CHANGE
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif !defined (UNIX_OR_WINDOWS)
#  error target system not defined.
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Set IRMP_LOGGING to 1 if want to log 0/1 data to UART
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef IRMP_LOGGING
#  define IRMP_LOGGING                          0       // 1: log IR signal (scan), 0: do not. default is 0
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Use external logging routines
 * If you enable external logging, you have also to enable IRMP_LOGGING above
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef IRMP_EXT_LOGGING
#  define IRMP_EXT_LOGGING                      0       // 1: use external logging, 0: do not. default is 0
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Set IRMP_PROTOCOL_NAMES to 1 if want to access protocol names (for logging etc), costs ~300 bytes RAM!
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef IRMP_PROTOCOL_NAMES
#  define IRMP_PROTOCOL_NAMES                   0       // 1: access protocol names, 0: do not. default is 0
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Use Callbacks to indicate input signal
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef IRMP_USE_CALLBACK
#  define IRMP_USE_CALLBACK                     0       // 1: use callbacks. 0: do not. default is 0
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Call the user-provided irmp_idle() function when IRMP is idle.
 * Can be used to disable the timer irq and enter a sleep mode to save power
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef IRMP_USE_IDLE_CALL
#  define IRMP_USE_IDLE_CALL                    0       // 1: use idle calls. 0: do not. default is 0
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Use Callback if complete data was received
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef IRMP_USE_COMPLETE_CALLBACK
#  define IRMP_USE_COMPLETE_CALLBACK           0       // 1: use callback. 0: do not. default is 0
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Enable PinChangeInterrupt add on for irmp_ISR(). Tested for NEC, Kaseiko, Denon, RC6 and Arduino ATMega
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef IRMP_ENABLE_PIN_CHANGE_INTERRUPT
#  define IRMP_ENABLE_PIN_CHANGE_INTERRUPT     0       // 1: enable PCI add on. 0: do not. default is 0
#endif

#if (IRMP_ENABLE_PIN_CHANGE_INTERRUPT == 1)
#  undef F_INTERRUPTS
#  define F_INTERRUPTS                          15625   // 15625 interrupts per second gives 64 us period
#endif
/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Use ChibiOS Events to signal that valid IR data was received
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#if defined(_CHIBIOS_RT_) || defined(_CHIBIOS_NIL_)

#  ifndef IRMP_USE_EVENT
#    define IRMP_USE_EVENT                      0       // 1: use event. 0: do not. default is 0
#  endif

#  if IRMP_USE_EVENT == 1 && !defined(IRMP_EVENT_BIT)
#    define IRMP_EVENT_BIT                      1                     // event flag or bit to send
#  endif
#  if IRMP_USE_EVENT == 1 && !defined(IRMP_EVENT_THREAD_PTR)
#    define IRMP_EVENT_THREAD_PTR               ir_receive_thread_p   // pointer to the thread to send the event to
extern thread_t *IRMP_EVENT_THREAD_PTR;                               // the pointer must be defined and initialized elsewhere
#  endif

#endif // _CHIBIOS_RT_ || _CHIBIOS_NIL_

#endif // IRMPCONFIG_STAGE2_H

