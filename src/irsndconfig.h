/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * irsndconfig.h
 *
 * DO NOT INCLUDE THIS FILE, WILL BE INCLUDED BY IRSND.H!
 *
 * Copyright (c) 2010-2019 Frank Meyer - frank(at)fli4l.de
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

#ifndef IRSNDCONFIG_H
#define IRSNDCONFIG_H

#if !defined(IRSND_H)
#  error please include only irsnd.h, not irsndconfig.h
#endif

// #define IRSND_DEBUG 1                                // activate debugging

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * F_INTERRUPTS: number of interrupts per second, should be in the range from 10000 to 20000, typically 15000
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef F_INTERRUPTS
#  define F_INTERRUPTS                          15000   // interrupts per second
#endif

// #define IR_OUTPUT_IS_ACTIVE_LOW

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change hardware pin here for Arduino IDE if no IRSND_OUTPUT_PIN specified
 * Should be first, since it covers multiple platforms
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#if defined(ARDUINO)                                                        // Arduino IDE for different platforms
#  if defined(ALLOW_DYNAMIC_PINS)
#    undef IRSND_OUTPUT_PIN
#    define IRSND_OUTPUT_PIN irsnd_output_pin
#  else // defined(ALLOW_DYNAMIC_PINS)
#    if !defined(IRSND_OUTPUT_PIN)                                            // Arduino IDE uses IRSND_OUTPUT_PIN instead of PORT and BIT
#      define IRSND_OUTPUT_PIN 4
#    endif
#  endif
#  if !defined(IRMP_TIMING_TEST_PIN)                                        // Only for test purposes
#    define IRMP_TIMING_TEST_PIN 5
#  endif

#if ! defined(IRSND_IR_FREQUENCY)
#define IRSND_IR_FREQUENCY                      38000
#endif
#define IRSND_INTERRUPT_FREQUENCY               (IRSND_IR_FREQUENCY * 2)  // *2 to toggle output pin at each interrupt
/*
 * For Arduino AVR use timer 2 with FIXED 38000 * 2 = 76000 interrupts per second to toggle output pin.
 * ISR is called each 4. interrupt at a rate of 19000 interrupts per second.
 */
#  undef  F_INTERRUPTS
#  define F_INTERRUPTS                          (IRSND_IR_FREQUENCY / 2)   // 19000 interrupts per second

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * AVR XMega section:
 *
 * Change hardware pin here:                    IRSND_XMEGA_OC0A = OC0A on ATxmegas  supporting OC0A, e.g. ATxmega128A1U
 *                                              IRSND_XMEGA_OC0B = OC0B on ATxmegas  supporting OC0B, e.g. ATxmega128A1U
 *                                              IRSND_XMEGA_OC0C = OC0C on ATxmegas  supporting OC0C, e.g. ATxmega128A1U
 *                                              IRSND_XMEGA_OC0D = OC0D on ATxmegas  supporting OC0D, e.g. ATxmega128A1U
 *                                              IRSND_XMEGA_OC1A = OC1A on ATxmegas  supporting OC1A, e.g. ATxmega128A1U
 *                                              IRSND_XMEGA_OC1B = OC1B on ATxmegas  supporting OC1B, e.g. ATxmega128A1U
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined(__AVR_XMEGA__)                                              // XMEGA
#  define IRSND_PORT_PRE                        PORTD
#  define XMEGA_Timer                           TCD0
#  define IRSND_OCx                             IRSND_XMEGA_OC0B        // use OC0B

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * AVR ATMega/ATTiny section:
 *
 * Change hardware pin here:                    IRSND_OC2  = OC2  on ATmegas         supporting OC2,  e.g. ATmega8
 *                                              IRSND_OC2A = OC2A on ATmegas         supporting OC2A, e.g. ATmega88
 *                                              IRSND_OC2B = OC2B on ATmegas         supporting OC2B, e.g. ATmega88
 *                                              IRSND_OC0  = OC0  on ATmegas         supporting OC0,  e.g. ATmega162
 *                                              IRSND_OC0A = OC0A on ATmegas/ATtinys supporting OC0A, e.g. ATtiny84, ATtiny85, ATtiny87/167
 *                                              IRSND_OC0B = OC0B on ATmegas/ATtinys supporting OC0B, e.g. ATtiny84, ATtiny85
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined(ATMEL_AVR)
#  define IRSND_OCx                             IRSND_OC2B              // use OC2B

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * PIC C18 or XC8 section:
 *
 * Change hardware pin here:                    IRSND_PIC_CCP1 = RC2 on PIC 18F2550/18F4550, ...
 *                                              IRSND_PIC_CCP2 = RC1 on PIC 18F2550/18F4550, ...
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined(PIC_C18)                                                  // C18 or XC8 compiler
#  if defined(__12F1840)                                                // XC8 compiler
#    define Pre_Scaler                          1                       // define prescaler for timer2 e.g. 1,4,16
#    define F_CPU                               32000000UL              // PIC frequency: set your freq here
#    define PIC_Scaler                          2                       // PIC needs /2 extra in IRSND_FREQ_32_KHZ calculation for right value

#  else                                                                 // C18 compiler
#    define IRSND_OCx                           IRSND_PIC_CCP2          // Use PWMx for PIC
                                                                        // change other PIC C18 specific settings:
#    define F_CPU                               48000000UL              // PIC frequency: set your freq here
#    define Pre_Scaler                          4                       // define prescaler for timer2 e.g. 1,4,16
#    define PIC_Scaler                          2                       // PIC needs /2 extra in IRSND_FREQ_32_KHZ calculation for right value
#  endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * ARM STM32 section:
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined (ARM_STM32)                                               // use B6 as IR output on STM32
#  define IRSND_PORT_LETTER                     B
#  define IRSND_BIT_NUMBER                      6
#  define IRSND_TIMER_NUMBER                    4
#  define IRSND_TIMER_CHANNEL_NUMBER            1                       // only channel 1 can be used at the moment, others won't work

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * ARM STM32 with HAL section - don't change here, define IRSND_Transmit_GPIO_Port & IRSND_Transmit_Pin in STM32Cube (Main.h)
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined (ARM_STM32_HAL)                                           // IRSND_Transmit_GPIO_Port & IRSND_Transmit_Pin must be defined in STM32Cube
#  define IRSND_PORT_LETTER                     IRSND_Transmit_GPIO_Port//Port of Transmit PWM Pin e.g.
#  define IRSND_BIT_NUMBER                      IRSND_Transmit_Pin      //Pim of Transmit PWM Pin e.g.
#  define IRSND_TIMER_HANDLER                   htim2                   //Handler of Timer e.g. htim (see tim.h)
#  define IRSND_TIMER_CHANNEL_NUMBER            TIM_CHANNEL_2           //Channel of the used Timer PWM Pin e.g. TIM_CHANNEL_2
#  define IRSND_TIMER_SPEED_APBX                64000000                //Speed of the corresponding APBx. (see STM32CubeMX: Clock Configuration)

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Teensy 3.x with teensyduino gcc compiler
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined (TEENSY_ARM_CORTEX_M4)
#  define IRSND_PIN                             5                       // choose an arduino pin with PWM function!

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * ESP8266 (Arduino, see IRSEND.ino)
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif defined (__xtensa__)
#  define IRSND_PIN                             0                       // choose an arduino pin with PWM function!

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Other target systems
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#elif !defined (UNIX_OR_WINDOWS)
#  error target system not defined.
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Use Callbacks to indicate output signal or something else
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef IRSND_USE_CALLBACK
#  define IRSND_USE_CALLBACK                    0                       // flag: 0 = don't use callbacks, 1 = use callbacks, default is 0
#endif

#endif // IRSNDCONFIG_H
