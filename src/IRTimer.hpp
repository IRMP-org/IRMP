/*
 * IRTimer.hpp
 *
 *  Copyright (C) 2020  Armin Joachimsmeyer
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

/*
 * We use IR timer (timer 2 for AVR) for receive and send. Both functions can be used alternating but not at the same time.
 * For receive we initialize IR timer to generate interrupts at 10 to 20 kHz for calling irmp_ISR().
 * For send we have 76 kHz to toggle output pin. The irsnd_ISR() call rate is 1/4 of IR signal toggle rate.
 * For send, initIRTimer() is called at each irsnd_send_data().
 * The current state of IR timer is stored by initIRTimer() and restored after sending.
 * This enables us to set up IR timer for receiving and on calling irsnd_send_data() the IR timer is reconfigured for the duration of sending.
 * Therefore no (non interrupt) receiving is possible during sending of data.
 */

// NO GUARD here, we have the GUARD below with #ifdef _IRSND_H_ and #ifdef _IRMP_H_.
#include "IRTimer.h"

#if !defined(TIMER_DECLARED)
#define TIMER_DECLARED
#  if defined(ESP32)
static hw_timer_t *sReceiveAndSendInterruptTimer = NULL;

// BluePill in 2 flavors see https://samuelpinches.com.au/3d-printer/cutting-through-some-confusion-on-stm32-and-arduino/
#  elif defined(__STM32F1__) || defined(ARDUINO_ARCH_STM32F1) // Recommended original Arduino_STM32 by Roger Clark.
// STM32F1 architecture for "Generic STM32F103C series" from "STM32F1 Boards (Arduino_STM32)" of Arduino Board manager
// http://dan.drown.org/stm32duino/package_STM32duino_index.json
#include <HardwareTimer.h> // 4 timers and 4. timer (4.channel) is used for tone()
/*
 * Use timer 3 as IRMP timer.
 * Timer 3 blocks PA6, PA7, PB0, PB1, so if you require one of them as tone() or Servo output, you must choose another timer.
 */
HardwareTimer sReceiveAndSendInterruptTimer(3);

#  elif defined(STM32F1xx) || defined(ARDUINO_ARCH_STM32) // STM32duino by ST Microsystems.
// stm32 architecture for "Generic STM32F1 series" from "STM32 Boards (selected from submenu)" of Arduino Board manager
// https://github.com/stm32duino/BoardManagerFiles/raw/master/STM32/package_stm_index.json
#include <HardwareTimer.h>
/*
 * Use timer 4 as IRMP timer.
 * Timer 4 blocks PB6, PB7, PB8, PB9, so if you require one of them as tone() or Servo output, you must choose another timer.
 */
#    if defined(TIM4)
HardwareTimer sReceiveAndSendInterruptTimer(TIM4);
#    else
HardwareTimer sReceiveAndSendInterruptTimer(TIM2);
#    endif

#elif defined(ARDUINO_ARCH_MBED) // Arduino Nano 33 BLE + Sparkfun Apollo3
mbed::Ticker sReceiveAndSendInterruptTimer;

/*
 * RP2040 based boards for pico core
 * https://github.com/earlephilhower/arduino-pico
 * https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
 * Can use any pin for PWM, no timer restrictions
 */
#elif defined(ARDUINO_ARCH_RP2040) // Raspberry Pi Pico, Adafruit Feather RP2040, etc.
#include "pico/time.h"
repeating_timer_t sReceiveAndSendInterruptTimer;
bool IRTimerInterruptHandlerHelper(repeating_timer_t*);

#elif defined(TEENSYDUINO)
// common for all Teensy
IntervalTimer sReceiveAndSendInterruptTimer;

#  endif
#endif // TIMER_DECLARED

#if defined(_IRMP_H_)
// we compile for irmp
#undef IR_INTERRUPT_FREQUENCY
#define IR_INTERRUPT_FREQUENCY      F_INTERRUPTS                // define frequency for receive

#elif defined(_IRSND_H_)
// we compile for irsnd
#undef IR_INTERRUPT_FREQUENCY
#define IR_INTERRUPT_FREQUENCY      IRSND_INTERRUPT_FREQUENCY   // define frequency for send

#endif // defined(_IRMP_H_)

// The eclipse formatter has problems with // comments in undefined code blocks
// !!! Must be without comment and closed by @formatter:on
// @formatter:off
#if defined(_IRMP_H_)
void initIRTimerForReceive(void)
#elif defined(_IRSND_H_)
void initIRTimerForSend(void)
#endif

{
#if defined(__AVR__)
// Use Timer 2
#  if defined(__AVR_ATmega16__)
    TCCR2 = _BV(WGM21) | _BV(CS21);                                 // CTC mode, prescale by 8
    OCR2 = (((F_CPU / 8) + (IR_INTERRUPT_FREQUENCY / 2)) / IR_INTERRUPT_FREQUENCY) - 1; // 132 for 15 kHz @16 MHz, 52 for 38 kHz @16 MHz
    TCNT2 = 0;
    TIMSK = _BV(OCIE2);                                             // enable interrupt

#  elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
// Since the ISR takes 5 to 22 microseconds for ATtiny@16MHz only 16 and 8 MHz makes sense
#    if defined(ARDUINO_AVR_DIGISPARK)
    // standard Digispark settings use timer 1 for millis() and micros()
// Timer 0 has only 1 and 8 as useful prescaler
    TCCR0A = 0;                                                     // must be set to zero before configuration!
#      if (F_CPU / IR_INTERRUPT_FREQUENCY) > 256                    // for 8 bit timer
    OCR0A = OCR0B = (((F_CPU / 8) + (IR_INTERRUPT_FREQUENCY / 2)) / IR_INTERRUPT_FREQUENCY) - 1; // 132 for 15 kHz @16 MHz, 52 for 38 kHz @16 MHz
    TCCR0B = _BV(CS01);                                             // presc = 8
#      else
    OCR0A = OCR0B = (F_CPU / IR_INTERRUPT_FREQUENCY) - 1;           // compare value: 209 for 76 kHz, 221 for 72kHz @16MHz
    TCCR0B = _BV(CS00);                                             // presc = 1 / no prescaling
#      endif
    TCCR0A = _BV(WGM01);                                            // CTC with OCRA as top
    TCNT0 = 0;
    TIMSK |= _BV(OCIE0B);                                           // enable compare match interrupt

#    else
// Use timer 1
#      if (F_CPU / IR_INTERRUPT_FREQUENCY) > 256                    // for 8 bit timer
    OCR1B = OCR1C = (((F_CPU / 8) + (IR_INTERRUPT_FREQUENCY / 2)) / IR_INTERRUPT_FREQUENCY) - 1; // 132 for 15 kHz @16 MHz, 52 for 38 kHz @16 MHz
    TCCR1 = _BV(CTC1) | _BV(CS12);                                  // switch CTC Mode on, set prescaler to 8
#      else
    OCR1B = OCR1C = (F_CPU / IR_INTERRUPT_FREQUENCY) - 1;           // compare value: 209 for 76 kHz, 221 for 72kHz @16MHz
    TCCR1 = _BV(CTC1) | _BV(CS10);                                  // switch CTC Mode on, set prescaler to 1 / no prescaling
#      endif
    TCNT1 = 0;
    TIMSK |= _BV(OCIE1B);                                           // enable compare match interrupt
#    endif

#  elif defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
// Timer 1 is a 16 bit counter so we need no prescaler
    ICR1 = (F_CPU / IR_INTERRUPT_FREQUENCY) - 1;                    // 1065 for 15 kHz @16 MHz. compare value: 1/15000 of CPU frequency
    TCCR1B = _BV(WGM12) | _BV(WGM13) | _BV(CS10);                   // switch CTC Mode on, set prescaler to 1 / no prescaling
    TCNT1 = 0;
    TIMSK1 = _BV(OCIE1B);                                           // enable compare match B interrupt, compare match A is used by tone library

#  elif defined(__AVR_ATmega4809__) // Uno WiFi Rev 2, Nano Every
    // TCB1 is used by Tone()
    // TCB2 is used by Servo
    // TCB3 is used by millis()
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
    TCB0.CCMP = (F_CPU / IR_INTERRUPT_FREQUENCY) - 1;               // compare value: 209 for 76 kHz, 221 for 72kHz @16MHz
    TCB0.INTFLAGS = TCB_CAPT_bm;                                    // reset interrupt flags
    TCB0.INTCTRL = TCB_CAPT_bm;                                     // enable capture compare interrupt
    TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm;

#  elif defined(__AVR_ATtiny1616__)  || defined(__AVR_ATtiny3216__) || defined(__AVR_ATtiny3217__) // TinyCore boards
    // use one ramp mode and overflow interrupt
#    if defined(MILLIS_USE_TIMERA0)
    TCD0.CTRLA = 0;                                                 // reset enable bit in order to unprotect the other bits
    TCD0.CTRLB = TCD_WGMODE_ONERAMP_gc;                             // must be set since it is used by PWM
//    TCD0.CMPBSET = 80;
    TCD0.CMPBCLR = (F_CPU / IR_INTERRUPT_FREQUENCY) - 1;            // 1332 for 15 kHz, 262 for 76000 interrupts per second @ 20MHz

    // Generate 50% duty cycle signal for debugging etc.
//    TCD0.CMPASET = 0;
//    TCD0.CMPACLR = (F_CPU / (IR_INTERRUPT_FREQUENCY * 2)) - 1;      // 50% duty cycle for WOA
//    TCD0.CTRLC = 0;                                                 // reset WOx outputs

//    _PROTECTED_WRITE(TCD0.FAULTCTRL,FUSE_CMPAEN_bm);                // enable WOA signal
//    PORTA.DIRSET = PIN4_bm;                                         // enable WOA output pin 13/PA4
//    _PROTECTED_WRITE(TCD0.FAULTCTRL,FUSE_CMPAEN_bm | FUSE_CMPBEN_bm); // enable WOA + WOB signal signal
//    PORTA.DIRSET = PIN4_bm | PIN5_bm;                               // enable WOA + WOB output pins 13/PA4 + 14/PA5

    TCD0.INTFLAGS = TCD_OVF_bm;                                     // reset interrupt flags
    TCD0.INTCTRL = TCD_OVF_bm;                                      // overflow interrupt
    // check enable ready
//    while ((TCD0.STATUS & TCD_ENRDY_bm) == 0); // Wait for Enable Ready to be high - I guess it is not required
    // enable timer - this locks the other bits and static registers and activates values in double buffered registers
    TCD0.CTRLA = TCD_ENABLE_bm | TCD_CLKSEL_SYSCLK_gc| TCD_CNTPRES_DIV1_gc; // System clock, no prescale, no synchronization prescaler
#    else
    TCA0.SINGLE.CTRLD = 0;                                      // Single mode - required at least for MegaTinyCore
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;            // Normal mode, top = PER
    TCA0.SINGLE.PER = (F_CPU / IR_INTERRUPT_FREQUENCY) - 1;     // 1332 for 15 kHz, 262 for 76000 interrupts per second @ 20MHz

    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;                   // reset interrupt flags
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;                    // overflow interrupt
    // check enable ready
//    while ((TCA0.SINGLE.STATUS & TCD_ENRDY_bm) == 0); // Wait for Enable Ready to be high - I guess it is not required
    // enable timer - this locks the other bits and static registers and activates values in double buffered registers
    TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm | TCA_SINGLE_CLKSEL_DIV1_gc; // System clock
#    endif
#  elif defined(__AVR_ATmega8__)
#    if (F_CPU / IR_INTERRUPT_FREQUENCY) <= 256                     // for 8 bit timer
    TCCR2 = _BV(WGM21) | _BV(CS20);                                 // CTC mode, no prescale
    OCR2 = (F_CPU / IR_INTERRUPT_FREQUENCY) - 1;                    // 209 for 76000 interrupts per second
#    else
    TCCR2 = _BV(WGM21) | _BV(CS21);                                 // CTC mode, prescale by 8
    OCR2 = (((F_CPU / 8) + (IR_INTERRUPT_FREQUENCY / 2)) / IR_INTERRUPT_FREQUENCY) - 1; // 132 for 15 kHz @16 MHz, 52 for 38 kHz @16 MHz
#    endif
    TCNT2 = 0;
    TIFR = _BV(OCF2) | _BV(TOV2);                                   // reset interrupt flags
    TIMSK = _BV(OCIE2);                                             // enable TIMER2_COMP_vect interrupt to be compatible with tone() library

#  elif defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega8U2__) || defined(__AVR_ATmega16U2__)  || defined(__AVR_ATmega32U2__) // Leonardo etc.
    TCCR3A = 0;
    TCCR3B = _BV(CS30) | _BV(WGM32);                                // no prescale, CTC mode Top OCR3A
    // Set OCR3B = OCR3A since we use TIMER3_COMPB_vect as interrupt, but run timer in CTC mode with OCR3A as TOP
    OCR3B = OCR3A = (F_CPU / IR_INTERRUPT_FREQUENCY) - 1;           // 1065 for 15 kHz, 209 for 76 kHz @ 16MHz
    TCNT3 = 0;
    TIMSK3 = _BV(OCIE3B);                                           // enable TIMER3_COMPB_vect interrupt to be compatible with tone() library

#  elif defined(OCF2B)  // __AVR_ATmega328__ here
    TCCR2A = _BV(WGM21);                                            // CTC mode
#    if (F_CPU / IR_INTERRUPT_FREQUENCY) <= 256                     // for 8 bit timer
    TCCR2B = _BV(CS20);                                             // no prescale
    // Set OCR2B = OCR2A since we use TIMER2_COMPB_vect as interrupt, but run timer in CTC mode with OCR2A as TOP
    OCR2B = OCR2A = (F_CPU / IR_INTERRUPT_FREQUENCY) - 1;           // 209 for 76000 interrupts per second @ 16MHz
#    else
    TCCR2B = _BV(CS21);                                             // prescale by 8
    OCR2B = OCR2A = (((F_CPU / 8) + (IR_INTERRUPT_FREQUENCY / 2)) / IR_INTERRUPT_FREQUENCY) - 1; // 132 for 15 kHz @16 MHz, 52 for 38 kHz @16 MHz
#    endif
    TCNT2 = 0;
    TIFR2 = _BV(OCF2B) | _BV(OCF2A) | _BV(TOV2);                    // reset interrupt flags
    TIMSK2 = _BV(OCIE2B);                                           // enable TIMER2_COMPB_vect interrupt to be compatible with tone() library

#  elif defined(TCCR1B)  // __AVR_ATtiny88__ here
    TCCR1A = 0;
    TCCR1B = _BV(WGM12) | _BV(CS10);                                // CTC mode, no prescaling
    OCR1A = (F_CPU / IR_INTERRUPT_FREQUENCY) - 1;                   // 209 for 76000 interrupts per second @ 16MHz
    TCNT1 = 0;
#    if defined(TIMSK1)
    TIMSK1 = _BV(OCIE1A);                                           // Timer/Counter1, Output Compare A Match Interrupt Enable
#    else // ATmega128 ?
    TIMSK = _BV(OCIE1A);                                            // Timer/Counter1, Output Compare A Match Interrupt Enable
#    endif

#  else // if defined(__AVR_ATmega16__) etc
#error "This AVR CPU is not supported by IRMP"
#  endif // if defined(__AVR_ATmega16__)

#elif defined(ESP8266)
    timer1_isr_init();
    timer1_attachInterrupt(irmp_timer_ISR);
    /*
     * TIM_DIV1 = 0,   //80MHz (80 ticks/us - 104857.588 us max)
     * TIM_DIV16 = 1,  //5MHz (5 ticks/us - 1677721.4 us max)
     * TIM_DIV256 = 3 //312.5Khz (1 tick = 3.2us - 26843542.4 us max)
     */
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP); // must be before timer1_write!
    timer1_write(((80000000 / 16) + (IR_INTERRUPT_FREQUENCY / 2)) / IR_INTERRUPT_FREQUENCY); // 80000000 holds for 80 and 160 MHz clock!

#elif defined(ESP32)
    // Tasmota requires timer 3 (last of 4 timers)
    // Use timer with 1 microsecond resolution, main clock is 80MHZ
    if(sReceiveAndSendInterruptTimer == NULL) {
#  if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        sReceiveAndSendInterruptTimer = timerBegin(1000000); // Only 1 parameter is required. 1000000 corresponds to 1 MHz / 1 uSec. After successful setup the timer will automatically start.
        timerAttachInterrupt(sReceiveAndSendInterruptTimer, irmp_timer_ISR);
        timerAlarm(sReceiveAndSendInterruptTimer, ((getApbFrequency() / 80) + (IR_INTERRUPT_FREQUENCY / 2)) / IR_INTERRUPT_FREQUENCY, true, 0); // 0 in the last parameter is repeat forever
#  else
        sReceiveAndSendInterruptTimer = timerBegin(3, 80, true);
        timerAttachInterrupt(sReceiveAndSendInterruptTimer, irmp_timer_ISR, false); // false -> level interrupt, true -> edge interrupt, but this is not supported :-(
        timerAlarmWrite(sReceiveAndSendInterruptTimer, ((getApbFrequency() / 80) + (IR_INTERRUPT_FREQUENCY / 2)) / IR_INTERRUPT_FREQUENCY, true);
#endif
    }
    enableIRTimerInterrupt();

#  if defined(DEBUG) && defined(ESP32)
    Serial.print("CPU frequency=");
    Serial.print(getCpuFrequencyMhz());
    Serial.println("MHz");
    Serial.print("Timer clock frequency=");
    Serial.print(getApbFrequency());
    Serial.println("Hz");
#  endif

// BluePill in 2 flavors
#elif defined(__STM32F1__) || defined(ARDUINO_ARCH_STM32F1) // Recommended original Arduino_STM32 by Roger Clark.
    // http://dan.drown.org/stm32duino/package_STM32duino_index.json
    sReceiveAndSendInterruptTimer.setMode(TIMER_CH1, TIMER_OUTPUT_COMPARE);
    sReceiveAndSendInterruptTimer.setPrescaleFactor(1);
    sReceiveAndSendInterruptTimer.setOverflow(F_CPU / IR_INTERRUPT_FREQUENCY);
    //sReceiveAndSendInterruptTimer.setPeriod(1000000 / IR_INTERRUPT_FREQUENCY);
    sReceiveAndSendInterruptTimer.attachInterrupt(TIMER_CH1, irmp_timer_ISR);
    sReceiveAndSendInterruptTimer.refresh();                                  // Set the timer's count to 0 and update the prescaler and overflow values.

#elif defined(STM32F1xx) || defined(ARDUINO_ARCH_STM32) // STM32duino by ST Microsystems.
    // https://github.com/stm32duino/BoardManagerFiles/raw/master/STM32/package_stm_index.json
    sReceiveAndSendInterruptTimer.setMode(LL_TIM_CHANNEL_CH1, TIMER_OUTPUT_COMPARE, NC);              // used for generating only interrupts, no pin specified
    sReceiveAndSendInterruptTimer.setPrescaleFactor(1);
    sReceiveAndSendInterruptTimer.setOverflow(F_CPU / IR_INTERRUPT_FREQUENCY, TICK_FORMAT);           // clock cycles period
    //sReceiveAndSendInterruptTimer.setOverflow(1000000 / IR_INTERRUPT_FREQUENCY, MICROSEC_FORMAT);   // microsecond period
    sReceiveAndSendInterruptTimer.attachInterrupt(irmp_timer_ISR);                                    // this sets update interrupt enable
    sReceiveAndSendInterruptTimer.resume();                           // Start or resume HardwareTimer: all channels are resumed, interrupts are enabled if necessary

#elif defined(ARDUINO_ARCH_SAMD)
    REG_GCLK_CLKCTRL = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TCC2_TC3); // GCLK1=32kHz,  GCLK0=48MHz
//    while (GCLK->STATUS.bit.SYNCBUSY) // not required to wait
//        ;

    TcCount16* TC = (TcCount16*) TC3;

    TC->CTRLA.reg &= ~TC_CTRLA_ENABLE;          // Enable write access to CTRLA register
    while (TC->STATUS.bit.SYNCBUSY == 1);       // wait for sync

// Set Timer counter Mode to 16 bits, use match mode so that the timer counter resets when the count matches the compare register
    TC->CTRLA.reg |= TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_MFRQ |TC_CTRLA_PRESCALER_DIV1;

    TC->CC[0].reg = (uint16_t) ((F_CPU / IR_INTERRUPT_FREQUENCY) - 1);   // ((48MHz / sampleRate) - 1);

// Enable the compare interrupt
    TC->INTENSET.reg = 0;
    TC->INTENSET.bit.MC0 = 1;

    NVIC_EnableIRQ (TC3_IRQn);

    TC->CTRLA.reg |= TC_CTRLA_ENABLE;
//    while (TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY); // Not required to wait at end of function

//#elif defined(ARDUINO_ARCH_APOLLO3)
//// Use Timer 3 segment B
//    am_hal_ctimer_clear(3, AM_HAL_CTIMER_TIMERB);   // reset timer
//// only AM_HAL_CTIMER_FN_REPEAT resets counter after match (CTC mode)
//    am_hal_ctimer_config_single(3, AM_HAL_CTIMER_TIMERB, (AM_HAL_CTIMER_INT_ENABLE | AM_HAL_CTIMER_HFRC_12MHZ | AM_HAL_CTIMER_FN_REPEAT));
//    am_hal_ctimer_compare_set(3, AM_HAL_CTIMER_TIMERB, 0, 12000000 / IR_INTERRUPT_FREQUENCY);
//    am_hal_ctimer_start(3, AM_HAL_CTIMER_TIMERB);
//
//    am_hal_ctimer_int_register(AM_HAL_CTIMER_INT_TIMERB3, irmp_timer_ISR);
//    am_hal_ctimer_int_enable(AM_HAL_CTIMER_INT_TIMERB3);
//    NVIC_EnableIRQ(CTIMER_IRQn);

#elif defined(ARDUINO_ARCH_MBED)
    sReceiveAndSendInterruptTimer.attach(irmp_timer_ISR, std::chrono::microseconds(1000000 / IR_INTERRUPT_FREQUENCY));

#elif defined(ARDUINO_ARCH_RP2040) // Raspberry Pi Pico, Adafruit Feather RP2040, etc.
    add_repeating_timer_us(-1000000 / IR_INTERRUPT_FREQUENCY, IRTimerInterruptHandlerHelper, NULL, &sReceiveAndSendInterruptTimer); // 13.15 us

#elif defined(TEENSYDUINO)
    sReceiveAndSendInterruptTimer.begin(irmp_timer_ISR, 1000000 / IR_INTERRUPT_FREQUENCY);
#endif // defined(__AVR__)
}

// @formatter:on
#if !defined(TIMER_FUNCTIONS_DEFINED)
#define TIMER_FUNCTIONS_DEFINED
/** Temporarily storage for timer register*/
#if defined(__AVR__)
uint8_t sTimerTCCRA;
uint8_t sTimerTCCRB;
#  if defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__) || defined(__AVR_ATtiny1616__)  || defined(__AVR_ATtiny3216__) \
    || defined(__AVR_ATtiny3217__) || defined(__AVR_ATmega4809__)
uint16_t sTimerOCR; // we have a 12/16 bit timer
#  else
uint8_t sTimerOCR;
#  endif
uint8_t sTimerOCRB;
uint8_t sTimerTIMSK;

#elif defined(ESP8266)
uint32_t sTimerLoadValue;

#elif defined(ESP32)
uint64_t sTimerAlarmValue;

#elif defined(STM32F1xx) || defined(ARDUINO_ARCH_STM32) || defined(__STM32F1__)
uint32_t sTimerOverflowValue;

#elif defined(ARDUINO_ARCH_SAMD) // || defined(ARDUINO_ARCH_APOLLO3)
uint16_t sTimerCompareCapureValue;

#endif // defined(__AVR__)

/*
 * If we do not use receive, we have no timer defined at the first call of this function.
 * But for AVR saving the timer settings is possible anyway, since it only consists of saving registers.
 * This helps cooperation with other libraries using the same timer.
 */
void storeIRTimer(void) {
#if defined(__AVR_ATmega16__)
    sTimerTCCRA = TCCR2;
    sTimerOCR = OCR2;
    sTimerTIMSK = TIMSK;

#elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#  if defined(ARDUINO_AVR_DIGISPARK)
    sTimerTCCRA = TCCR0A;
    sTimerTCCRB = TCCR0B;
    sTimerOCRB = OCR0B;
    sTimerOCR = OCR0A;
    sTimerTIMSK = TIMSK;
#  else
    sTimerTCCRA = TCCR1;
    sTimerOCRB = OCR1B;
    sTimerOCR = OCR1C;
    sTimerTIMSK = TIMSK;
#  endif

#elif defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
    sTimerTCCRB = TCCR1B;
    sTimerOCR = ICR1;
    sTimerOCRB = OCR1B;
    sTimerTIMSK = TIMSK1;

#elif defined(__AVR_ATmega4809__) // Uno WiFi Rev 2, Nano Every
    // store current timer state
    sTimerTCCRA = TCB0.CTRLA;
    sTimerTCCRB = TCB0.CTRLB;
    sTimerOCR = TCB0.CCMP;
    sTimerTIMSK = TCB0.INTCTRL;

#elif defined(__AVR_ATmega8__)
    sTimerTCCRA = TCCR2;
    sTimerOCR = OCR2;
    sTimerTIMSK = TIMSK;

#elif defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega8U2__) || defined(__AVR_ATmega16U2__)  || defined(__AVR_ATmega32U2__) // Leonardo etc.
    // store current timer state
    sTimerTCCRA = TCCR3A;
    sTimerTCCRB = TCCR3B;
    sTimerOCR = OCR3A;
    sTimerOCRB = OCR3B;
    sTimerTIMSK = TIMSK3;

#elif defined(__AVR_ATtiny1616__)  || defined(__AVR_ATtiny3216__) || defined(__AVR_ATtiny3217__)
    // store settings used for PWM
#  if defined(MILLIS_USE_TIMERA0)
    sTimerTCCRA = TCD0.CTRLA;
    sTimerTCCRB = TCD0.CTRLB;
    sTimerOCR = TCD0.CMPBCLR;
    sTimerOCRB = TCD0.CTRLC;
    sTimerTIMSK = TCD0.INTCTRL;
#  else
    sTimerTCCRA = TCA0.SINGLE.CTRLA;
    sTimerTCCRB = TCA0.SINGLE.CTRLB;
    sTimerOCR = TCA0.SINGLE.PER;
    sTimerOCRB = TCA0.SINGLE.CTRLC;
    sTimerTIMSK = TCA0.SINGLE.INTCTRL;
#  endif

#elif defined(OCF2B)  // __AVR_ATmega328__ here
    // store current timer state
    sTimerTCCRA = TCCR2A;
    sTimerTCCRB = TCCR2B;
    sTimerOCR = OCR2A;
    sTimerOCRB = OCR2B;
    sTimerTIMSK = TIMSK2;

#elif defined(TCCR1B)  // __AVR_ATtiny88__ here
    // store current timer state
    sTimerTCCRA = TCCR1A;
    sTimerTCCRB = TCCR1B;
    sTimerOCR = OCR1A;
    sTimerOCRB = OCR1B;
#    if defined(TIMSK1)
    sTimerTIMSK = TIMSK1;
#    else
    sTimerTIMSK = TIMSK;
#    endif

#elif defined(__AVR__)
// #error "This AVR CPU is not supported by IRMP"

#elif defined(ESP8266)
    sTimerLoadValue= T1L;
#endif // defined(__AVR_ATmega16__)

#if defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND)
    // If we do not use receive, we have no timer defined at the first call of this function
#  if defined(ESP32)
#    if ESP_ARDUINO_VERSION < ESP_ARDUINO_VERSION_VAL(3, 0, 0)
            sTimerAlarmValue = timerAlarmRead(sReceiveAndSendInterruptTimer);
#    endif

#  elif defined(STM32F1xx)
    sTimerOverflowValue = sReceiveAndSendInterruptTimer.getOverflow(TICK_FORMAT);

#  elif defined(ARDUINO_ARCH_STM32) // Untested! use settings from BluePill / STM32F1xx
    sTimerOverflowValue = sReceiveAndSendInterruptTimer.getOverflow(TICK_FORMAT);

#  elif defined(__STM32F1__)
    sTimerOverflowValue = sReceiveAndSendInterruptTimer.getOverflow();

#  elif defined(ARDUINO_ARCH_SAMD)
    sTimerCompareCapureValue = TC3->COUNT16.CC[0].reg;

//#    elif defined(ARDUINO_ARCH_APOLLO3)
//    sTimerCompareCapureValue = *((uint32_t *)CTIMERADDRn(CTIMER, 3, CMPRB0)) & 0xFFFF;
#  endif
#endif // defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND)
}

/*
 * Restore settings of the timer e.g. for IRSND
 */
void restoreIRTimer(void) {
#if defined(__AVR_ATmega16__)
    TCCR2 = sTimerTCCRA;
    OCR2 = sTimerOCR;
    TIMSK = sTimerTIMSK;

#elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#  if defined(ARDUINO_AVR_DIGISPARK)
    TCCR0A = sTimerTCCRA;
    TCCR0B = sTimerTCCRB;
    OCR0B = sTimerOCRB;
    OCR0A = sTimerOCR;
    TIMSK = sTimerTIMSK;
#  else
    TCCR1 = sTimerTCCRA;
    OCR1B = sTimerOCRB;
    OCR1C = sTimerOCR;
    TIMSK = sTimerTIMSK;
#  endif

#elif  defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
    TCCR1B = sTimerTCCRB;
    ICR1 = sTimerOCR;
    OCR1B = sTimerOCRB;
    TIMSK1 = sTimerTIMSK;

#elif defined(__AVR_ATmega4809__) // Uno WiFi Rev 2, Nano Every
    TCB0.CTRLA = sTimerTCCRA;
    TCB0.CTRLB = sTimerTCCRB;
    TCB0.CCMP = sTimerOCR;
    TCB0.INTCTRL = sTimerTIMSK;

#elif defined(__AVR_ATmega8__)
    TCCR2 = sTimerTCCRA;
    OCR2 = sTimerOCR;
    TIMSK = sTimerTIMSK;

#elif defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega8U2__) || defined(__AVR_ATmega16U2__)  || defined(__AVR_ATmega32U2__) // Leonardo etc.
    // restore current timer state
    TCCR3A = sTimerTCCRA;
    TCCR3B = sTimerTCCRB;
    OCR3A = sTimerOCR;
    OCR3B = sTimerOCRB;
    TIMSK3 = sTimerTIMSK;

#elif defined(__AVR_ATtiny1616__)  || defined(__AVR_ATtiny3216__) || defined(__AVR_ATtiny3217__)
    // restore settings used for PWM
#    if defined(MILLIS_USE_TIMERA0)
    TCD0.CTRLA = 0; // unlock timer
    TCD0.CTRLB = sTimerTCCRB;
    TCD0.CMPBCLR = sTimerOCR;
    TCD0.CTRLC = sTimerOCRB;
    TCD0.INTCTRL = sTimerTIMSK;
    TCD0.CTRLA = sTimerTCCRA;
#    else
    TCA0.SINGLE.CTRLA = 0; // unlock timer
    TCA0.SINGLE.CTRLB = sTimerTCCRB;
    TCA0.SINGLE.PER = sTimerOCR;
    TCA0.SINGLE.CTRLC = sTimerOCRB;
    TCA0.SINGLE.INTCTRL = sTimerTIMSK;
    TCA0.SINGLE.CTRLA = sTimerTCCRA;
#    endif

#elif defined(OCF2B)  // __AVR_ATmega328__ here
    TCCR2A = sTimerTCCRA;
    TCCR2B = sTimerTCCRB;
    OCR2A = sTimerOCR;
    OCR2B = sTimerOCRB;
    TIMSK2 = sTimerTIMSK;

#elif defined(TCCR1B)  // __AVR_ATtiny88__ here
    // store current timer state
     TCCR1A = sTimerTCCRA;
     TCCR1B = sTimerTCCRB;
     OCR1A = sTimerOCR;
     OCR1B = sTimerOCRB;
#    if defined(TIMSK1)
     TIMSK1 = sTimerTIMSK;
#    else
     TIMSK = sTimerTIMSK;
#    endif
#elif defined(__AVR__)
// #error "This AVR CPU is not supported by IRMP"

#elif defined(ESP8266)
    timer1_write(sTimerLoadValue);
#endif // defined(__AVR_ATmega16__)

#if defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND)
#  if defined(ESP32)
#    if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    // we have no timerAlarmRead, so we just initialize timer again.
    timerAlarm(sReceiveAndSendInterruptTimer, ((getApbFrequency() / 80) + (IR_INTERRUPT_FREQUENCY / 2)) / IR_INTERRUPT_FREQUENCY, true, 0); // 0 in the last parameter is repeat forever
#    else
    timerAlarmWrite(sReceiveAndSendInterruptTimer, sTimerAlarmValue, true);
#    endif

#  elif defined(STM32F1xx)
    sReceiveAndSendInterruptTimer.setOverflow(sTimerOverflowValue, TICK_FORMAT);

#  elif defined(ARDUINO_ARCH_STM32) // Untested! use settings from BluePill / STM32F1xx
    sReceiveAndSendInterruptTimer.setOverflow(sTimerOverflowValue, TICK_FORMAT);

#  elif defined(__STM32F1__)
    sReceiveAndSendInterruptTimer.setOverflow(sTimerOverflowValue);

#  elif defined(ARDUINO_ARCH_SAMD)
    TC3->COUNT16.CC[0].reg = sTimerCompareCapureValue;

//#  elif defined(ARDUINO_ARCH_APOLLO3)
//    am_hal_ctimer_compare_set(3, AM_HAL_CTIMER_TIMERB, 0, sTimerCompareCapureValue);

#  elif defined(ARDUINO_ARCH_MBED)
    sReceiveAndSendInterruptTimer.attach(irmp_timer_ISR, std::chrono::microseconds(1000000 / IR_INTERRUPT_FREQUENCY));

#elif defined(ARDUINO_ARCH_RP2040)
    add_repeating_timer_us(-1000000 / IR_INTERRUPT_FREQUENCY, IRTimerInterruptHandlerHelper, NULL, &sReceiveAndSendInterruptTimer);

#  elif defined(TEENSYDUINO)
    sReceiveAndSendInterruptTimer.update(1000000 / IR_INTERRUPT_FREQUENCY);
#  endif
#endif // defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND)
}

/*
 * NOT used if IRMP_ENABLE_PIN_CHANGE_INTERRUPT is defined
 * Initialize timer to generate interrupts at a rate F_INTERRUPTS (15000) per second to poll the input pin.
 */
void disableIRTimerInterrupt(void) {
#if defined(__AVR__)
// Use Timer 2
#  if defined(__AVR_ATmega16__)
    TIMSK = 0;                  // disable interrupt

#  elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#    if defined(ARDUINO_AVR_DIGISPARK)
    TIMSK &= ~_BV(OCIE0B);      // disable interrupt
#    else
    TIMSK &= ~_BV(OCIE1B);      // disable interrupt
#    endif

#  elif  defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
    TIMSK1 &= ~_BV(OCIE1A);     // disable interrupt

#elif defined(__AVR_ATmega4809__) // Uno WiFi Rev 2, Nano Every
    TCB0.INTCTRL &= ~TCB_CAPT_bm;

#elif defined(__AVR_ATmega8__)
    TIMSK &= ~_BV(OCIE2);       // disable interrupt

#  elif defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega8U2__) || defined(__AVR_ATmega16U2__)  || defined(__AVR_ATmega32U2__) // Leonardo etc.
    TIMSK3 = 0;                 // disable interrupt

#  elif defined(__AVR_ATtiny1616__)  || defined(__AVR_ATtiny3216__) || defined(__AVR_ATtiny3217__)
#    if defined(MILLIS_USE_TIMERA0)
    TCD0.INTCTRL = 0;           // overflow interrupt
#    else
    TCA0.SINGLE.INTCTRL = 0;           // overflow interrupt
#    endif

#  elif defined(OCF2B)  // __AVR_ATmega328__ here
    TIMSK2 = 0; // disable interrupt

#  elif defined(TCCR1B)  // __AVR_ATtiny88__ here
#    if defined(TIMSK1)
    TIMSK1 = 0;
#    else
    TIMSK = 0;
#    endif

#  else
// #error "This AVR CPU is not supported by IRMP"

#  endif // defined(__AVR_ATmega16__)

#elif defined(ESP8266)
    timer1_detachInterrupt(); // disables interrupt too

#elif defined(ESP32)
    if (sReceiveAndSendInterruptTimer != NULL) {
#    if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        timerStop(sReceiveAndSendInterruptTimer);
#    else
            timerAlarmDisable(sReceiveAndSendInterruptTimer);
#    endif
    }

#elif defined(STM32F1xx) || defined(ARDUINO_ARCH_STM32)     // STM32duino by ST Microsystems.
    sReceiveAndSendInterruptTimer.setMode(LL_TIM_CHANNEL_CH1, TIMER_DISABLED);
    sReceiveAndSendInterruptTimer.detachInterrupt();

#elif defined(__STM32F1__) || defined(ARDUINO_ARCH_STM32F1) // Recommended original Arduino_STM32 by Roger Clark.
    sReceiveAndSendInterruptTimer.setMode(TIMER_CH1, TIMER_DISABLED);
    sReceiveAndSendInterruptTimer.detachInterrupt(TIMER_CH1);

#elif defined(ARDUINO_ARCH_SAMD)
    TC3->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
//    while (TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY) ; // Not required to wait at end of function

//#elif defined(ARDUINO_ARCH_APOLLO3)
//    am_hal_ctimer_int_disable(AM_HAL_CTIMER_INT_TIMERB3);

#elif defined(ARDUINO_ARCH_MBED)
    sReceiveAndSendInterruptTimer.detach();

#elif defined(ARDUINO_ARCH_RP2040)
    cancel_repeating_timer(&sReceiveAndSendInterruptTimer);

#elif defined(TEENSYDUINO)
    sReceiveAndSendInterruptTimer.end();
#endif // defined(__AVR__)
}

// used by AllProtocols example
void enableIRTimerInterrupt(void) {
#if defined(__AVR__)
// Use Timer 2
#  if defined(__AVR_ATmega16__)
    TIMSK = _BV(OCIE2);             // enable interrupt

#  elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#    if defined(ARDUINO_AVR_DIGISPARK)
    TIMSK |= _BV(OCIE0B);           // enable compare match interrupt
#    else
    TIMSK |= _BV(OCIE1B);           // enable compare match interrupt
#    endif

#  elif  defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
    TIMSK1 |= _BV(OCIE1A);          // enable compare match interrupt

#elif defined(__AVR_ATmega4809__)   // Uno WiFi Rev 2, Nano Every
    TCB0.INTCTRL = TCB_CAPT_bm;

#elif defined(__AVR_ATmega8__)
    TIMSK = _BV(OCIE2);             // enable interrupt

#  elif defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega8U2__) || defined(__AVR_ATmega16U2__)  || defined(__AVR_ATmega32U2__) // Leonardo etc.
    TIMSK3 = _BV(OCIE3B);           // enable interrupt

#  elif defined(__AVR_ATtiny1616__)  || defined(__AVR_ATtiny3216__) || defined(__AVR_ATtiny3217__)
#    if defined(MILLIS_USE_TIMERA0)
    TCD0.INTCTRL = TCD_OVF_bm;      // overflow interrupt
#    else
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;      // overflow interrupt
#    endif

#  elif defined(OCF2B)  // __AVR_ATmega328__ here
    TIMSK2 = _BV(OCIE2B); // enable interrupt

#  elif defined(TCCR1B)  // __AVR_ATtiny88__ here
#    if defined(TIMSK1)
    TIMSK1 = _BV(OCIE1A);
#    else
    TIMSK = _BV(OCIE1A);
#    endif

#  else
// #error "This AVR CPU is not supported by IRMP"

#  endif // defined(__AVR_ATmega16__)

#elif defined(ESP8266)
    timer1_attachInterrupt(irmp_timer_ISR); // enables interrupt too

#elif defined(ESP32)
    if (sReceiveAndSendInterruptTimer != NULL) {
#  if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)  // timerAlarm() enables it automatically
        timerStart(sReceiveAndSendInterruptTimer);
#  else
        timerAlarmEnable(sReceiveAndSendInterruptTimer);
#  endif
    }

#elif defined(__STM32F1__) || defined(ARDUINO_ARCH_STM32F1) // Recommended original Arduino_STM32 by Roger Clark.
    // http://dan.drown.org/stm32duino/package_STM32duino_index.json
    sReceiveAndSendInterruptTimer.setMode(TIMER_CH1, TIMER_OUTPUT_COMPARE);
    sReceiveAndSendInterruptTimer.attachInterrupt(TIMER_CH1, irmp_timer_ISR);
    sReceiveAndSendInterruptTimer.refresh(); // Set the timer's count to 0 and update the prescaler and overflow values.

#elif defined(STM32F1xx) || defined(ARDUINO_ARCH_STM32) // STM32duino by ST Microsystems.
    // https://github.com/stm32duino/BoardManagerFiles/raw/master/STM32/package_stm_index.json
    sReceiveAndSendInterruptTimer.setMode(LL_TIM_CHANNEL_CH1, TIMER_OUTPUT_COMPARE, NC); // used for generating only interrupts, no pin specified
    sReceiveAndSendInterruptTimer.attachInterrupt(irmp_timer_ISR);
    sReceiveAndSendInterruptTimer.refresh();// Set the timer's count to 0 and update the prescaler and overflow values.

#elif defined(ARDUINO_ARCH_SAMD)
    TC3->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
    while (TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
    ; //wait until TC5 is done syncing

//#elif defined(ARDUINO_ARCH_APOLLO3)
//    am_hal_ctimer_int_enable(AM_HAL_CTIMER_INT_TIMERB3);

#elif defined(ARDUINO_ARCH_MBED)
    sReceiveAndSendInterruptTimer.attach(irmp_timer_ISR, std::chrono::microseconds(1000000 / IR_INTERRUPT_FREQUENCY));

#elif defined(ARDUINO_ARCH_RP2040)
    add_repeating_timer_us(-1000000 / IR_INTERRUPT_FREQUENCY, IRTimerInterruptHandlerHelper, NULL, &sReceiveAndSendInterruptTimer);

#elif defined(TEENSYDUINO)
    sReceiveAndSendInterruptTimer.begin(irmp_timer_ISR, 1000000 / IR_INTERRUPT_FREQUENCY);
#else
#warning Board / CPU is not covered by definitions using pre-processor symbols -> no timer available. Please extend IRTimer.hpp.
#endif // defined(__AVR__)
}

#endif // TIMER_FUNCTIONS_DEFINED

/*
 * If both irmp and irsnd are used, compile it only once in the second step, when all variables are declared.
 */
#if (! defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND) || ( defined(_IRMP_ARDUINO_EXT_H) && defined(_IRSND_ARDUINO_EXT_H) )) && ! defined(ISR_DEFINED)
#  if !defined(ISR_DEFINED)
#define ISR_DEFINED
#  endif
/*
 * ISR is active while signal is sent AND during the trailing pause of IR frame
 * Called every 13.5us
 * Bit bang requires 5.9 us. 5 us for 16 push and 16 pop etc. and 0.9 us for function body
 * Together with call of irsnd_ISR() 10.5 us (frame) or 9.4 (trailing pause) - measured by scope
 * We use TIMER2_COMPB_vect to be compatible with tone() library
 */
#if defined(__AVR__)

#  if F_CPU < 8000000L
#error F_CPU must not be less than 8MHz for IRMP and IRSND
#  endif

#  if defined(__AVR_ATmega16__)
ISR(TIMER2_COMP_vect)
#  elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
#    if defined(ARDUINO_AVR_DIGISPARK)
ISR(TIMER0_COMPB_vect)
#    else
ISR(TIMER1_COMPB_vect)
#    endif

#  elif defined(__AVR_ATmega4809__) // Uno WiFi Rev 2, Nano Every
ISR(TCB0_INT_vect)

#  elif defined(__AVR_ATmega8__)
ISR(TIMER2_COMP_vect)

#  elif defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega8U2__) || defined(__AVR_ATmega16U2__)  || defined(__AVR_ATmega32U2__) // Leonardo etc.
ISR(TIMER3_COMPB_vect)

#  elif defined(__AVR_ATtiny1616__)  || defined(__AVR_ATtiny3216__) || defined(__AVR_ATtiny3217__)
#    if defined(MILLIS_USE_TIMERA0)
ISR(TCD0_OVF_vect)
#    else
ISR(TCA0_OVF_vect)
#    endif

#  elif defined(OCF2B)  // __AVR_ATmega328__ here
ISR(TIMER2_COMPB_vect)

#  elif defined(TCCR1B)  // __AVR_ATtiny88__ here
ISR(TIMER1_COMPB_vect)

#  else
// #error "This AVR CPU is not supported by IRMP"

#  endif // defined(__AVR_ATmega16__)

#elif defined(ESP8266) || defined(ESP32)
void IRAM_ATTR irmp_timer_ISR(void)

#elif defined(ARDUINO_ARCH_SAMD)
void TC3_Handler(void)

#elif defined(STM32F1xx) && STM32_CORE_VERSION_MAJOR == 1 &&  STM32_CORE_VERSION_MINOR <= 8 // for "Generic STM32F1 series" from "STM32 Boards (selected from submenu)" of Arduino Board manager
void irmp_timer_ISR(HardwareTimer *aDummy __attribute__((unused))) // old 1.8 version - changed in stm32duino 1.9 - 5/2020

#elif defined(ARDUINO_ARCH_RP2040)
void irmp_timer_ISR(void);
bool IRTimerInterruptHandlerHelper(repeating_timer_t*) { // we are called with a different signature
    irmp_timer_ISR();
    return true;
}
void irmp_timer_ISR(void)

#else // STM32F1xx (v1.9), __STM32F1__, ARDUINO_ARCH_APOLLO3, MBED, TEENSYDUINO
void irmp_timer_ISR(void)

#endif // defined(__AVR__)

// Start of ISR
{
#if defined(ARDUINO_ARCH_SAMD)
    TC3->COUNT16.INTFLAG.bit.MC0 = 1; // Clear interrupt
#endif

#if defined(__AVR_ATmega4809__)     // Uno WiFi Rev 2, Nano Every
    // Not tested, but with the experience, I made with the ATtiny3217, I guess it is required
    TCB0.INTFLAGS = TCB_CAPT_bm;    // reset interrupt flags
#endif
#if defined(__AVR_ATtiny1616__)  || defined(__AVR_ATtiny3216__) || defined(__AVR_ATtiny3217__)
    // must reset interrupt flag here
#  if defined(MILLIS_USE_TIMERA0)
    TCD0.INTFLAGS = TCD_OVF_bm;
#  else
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
#  endif
#endif

#if (defined(_IRSND_H_) || defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND))
    static uint8_t sDivider;        // IR signal toggle rate is 2 (4) times IRSND call rate
#endif

#if defined(IRMP_MEASURE_TIMING) && defined(IR_TIMING_TEST_PIN)
    digitalWriteFast(IR_TIMING_TEST_PIN, HIGH); // 2 clock cycles
#endif

#if defined(_IRSND_H_) || defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND)
    /*
     * Send part of ISR
     */
    if(irsnd_busy) {
        if (irsnd_is_on)
        {
#  if defined(IRSND_GENERATE_NO_SEND_RF)
            // output is active low
            if (__builtin_constant_p(IRSND_OUTPUT_PIN) ) { digitalWriteFast(IRSND_OUTPUT_PIN, IR_OUTPUT_ACTIVE_LEVEL);} else { digitalWrite(IRSND_OUTPUT_PIN, IR_OUTPUT_ACTIVE_LEVEL);}
#  else
            if(sDivider & 0x01) // true / inactive if sDivider is 3 or 1, so we start with active and end with inactive
            {
                if (__builtin_constant_p(IRSND_OUTPUT_PIN) ) { digitalWriteFast(IRSND_OUTPUT_PIN, IR_OUTPUT_INACTIVE_LEVEL);} else { digitalWrite(IRSND_OUTPUT_PIN, IR_OUTPUT_INACTIVE_LEVEL);}
            } else {
                if (__builtin_constant_p(IRSND_OUTPUT_PIN) ) { digitalWriteFast(IRSND_OUTPUT_PIN, IR_OUTPUT_ACTIVE_LEVEL);} else { digitalWrite(IRSND_OUTPUT_PIN, IR_OUTPUT_ACTIVE_LEVEL);}
            }


#  endif // defined(IRSND_GENERATE_NO_SEND_RF)
        } else {
            // irsnd off here
            if (__builtin_constant_p(IRSND_OUTPUT_PIN) ) { digitalWriteFast(IRSND_OUTPUT_PIN, IR_OUTPUT_INACTIVE_LEVEL);} else { digitalWrite(IRSND_OUTPUT_PIN, IR_OUTPUT_INACTIVE_LEVEL);}
        }

        /*
         * Call irsnd_ISR() every second (fourth) call if current LED state is inactive
         */
        if (--sDivider == 0)
        {
            // This in turn calls irsnd_on() or irsnd_off(). Empty call requires additional 0.7 us.
            if (!irsnd_ISR())
            {
                // End of frame
                restoreIRTimer();
#  if ! defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND)
// only send mode required -> disable interrupt
                disableIRTimerInterrupt();
#  endif
                sDivider = 1; // to call irsnd_ISR() directly at next interrupt
            } else {
                sDivider = 4;
            }
        }
    } // if(irsnd_busy)
#endif // defined(_IRSND_H_) || defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND)
#if defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND)
    else
    { // for receive and send in one ISR
#endif

#if defined(_IRMP_H_) || defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND)
    /*
     * Receive part of ISR
     */
    irmp_ISR();
#endif

#if defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND)
} // for receive and send in one ISR
#endif

#if defined(IRMP_MEASURE_TIMING) && defined(IR_TIMING_TEST_PIN)
    digitalWriteFast(IR_TIMING_TEST_PIN, LOW); // 2 clock cycles
#endif
}
#endif // (! defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND) || ( defined(_IRMP_ARDUINO_EXT_H) && defined(_IRSND_ARDUINO_EXT_H) )) && ! defined(ISR_DEFINED)
