/*
 * IRTimer.cpp.h
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

#ifndef TIMER_DECLARED
#define TIMER_DECLARED
#  if defined(ESP32)
static hw_timer_t * sESP32Timer = NULL;

// BluePill in 2 flavors
#  elif defined(STM32F1xx)   // for "Generic STM32F1 series" from STM32 Boards from STM32 cores of Arduino Board manager
#include <HardwareTimer.h> // 4 timers and 4. timer is used for tone()
/*
 * Use timer 3 as IRMP timer.
 * Timer 3 blocks PA6, PA7, PB0, PB1, so if you need one them as Servo output, you must choose another timer.
 */
HardwareTimer sSTM32Timer(TIM3);

#  elif defined(__STM32F1__) // for "Generic STM32F103C series" from STM32F1 Boards (STM32duino.com) of manual installed hardware folder
#include <HardwareTimer.h> // 4 timers and 4. timer is used for tone()
/*
 * Use timer 3 as IRMP timer.
 * Timer 3 blocks PA6, PA7, PB0, PB1, so if you need one them as tone() or Servo output, you must choose another timer.
 */
HardwareTimer sSTM32Timer(3);
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

/*
 * Temporarily storage for timer register
 */
#  if defined(__AVR__)
uint8_t sTimerTCCRA;
uint8_t sTimerTCCRB;
#    if defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
uint16_t sTimerOCR; // we have a 16 bit timer
#    else
uint8_t sTimerOCR;
#    endif
uint8_t sTimerOCRB;
uint8_t sTimerTIMSK;

#  elif defined(ESP8266)
uint32_t sTimerLoadValue;

#  elif defined(ESP32)
uint64_t sTimerAlarmValue;

#  elif defined(STM32F1xx) || defined(__STM32F1__)
uint32_t sTimerOverflowValue;

#  elif defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_ARCH_APOLLO3)
uint16_t sTimerCompareCapureValue;

#  endif // defined(__AVR__)
#endif // defined(_IRMP_H_)

#if defined(_IRMP_H_)
void initIRTimerForReceive(void)
#elif defined(_IRSND_H_)
void initIRTimerForSend(void)
#endif
{
#if defined(__AVR__)
// Use Timer 2
#  if defined(__AVR_ATmega16__)
    TCCR2 = _BV(WGM21) | _BV(CS21);                                     // CTC mode, prescale by 8
    OCR2 = ((F_CPU / 8) / IR_INTERRUPT_FREQUENCY) - 1;// 132 for 15000 interrupts per second
    TIMSK = _BV(OCIE2);// enable interrupt
    TCNT2 = 0;

#  elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
// Since the ISR takes 5 to 22 microseconds for ATtiny@16MHz only 16 and 8 MHz makes sense
#    if defined(ARDUINO_AVR_DIGISPARK)
// the digispark core uses timer 1 for millis() :-(
// Timer 0 has only 1 and 8 as useful prescaler
    TCCR0A = 0;// must be set to zero before configuration!
#      if (F_CPU / IR_INTERRUPT_FREQUENCY) > 256                        // for 8 bit timer
    OCR0A = OCR0B = ((F_CPU / 8) / IR_INTERRUPT_FREQUENCY) - 1;         // 132 for 15 kHz @16 MHz
    TCCR0B = _BV(CS01);// presc = 8
#      else
    OCR0A = OCR0B = (F_CPU / IR_INTERRUPT_FREQUENCY) - 1;               // compare value: 209 for 76 kHz, 221 for 72kHz @16MHz
    TCCR0B = _BV(CS00);// presc = 1 / no prescaling
#      endif
    TCCR0A = _BV(WGM01);                                                // CTC with OCRA as top
    TIMSK |= _BV(OCIE0B);// enable compare match interrupt
#    else
#      if (F_CPU / IR_INTERRUPT_FREQUENCY) > 256                    // for 8 bit timer
    OCR1B = OCR1C = ((F_CPU / 8) / IR_INTERRUPT_FREQUENCY) - 1;         // 132 for 15 kHz @16 MHz
    TCCR1 = _BV(CTC1) | _BV(CS12);// switch CTC Mode on, set prescaler to 8
#      else
    OCR1B = OCR1C = (F_CPU / IR_INTERRUPT_FREQUENCY) - 1;               // compare value: 209 for 76 kHz, 221 for 72kHz @16MHz
    TCCR1 = _BV(CTC1) | _BV(CS10);// switch CTC Mode on, set prescaler to 1 / no prescaling
#      endif
    TIMSK |= _BV(OCIE1B);                                               // enable compare match interrupt
#    endif

#  elif defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
// Timer 1 is a 16 bit counter so we need no prescaler
    ICR1 = (F_CPU / IR_INTERRUPT_FREQUENCY) - 1;// 1065 for 15 kHz @16 MHz. compare value: 1/15000 of CPU frequency
    TCCR1B = 0;                                 // switch CTC Mode on
    TCCR1B = _BV(WGM12) | _BV(WGM13) | _BV(CS10);// switch CTC Mode on, set prescaler to 1 / no prescaling
    TIMSK1 = _BV(OCIE1B);                       // enable compare match interrupt

#elif defined(__AVR_ATmega4809__) // Uno WiFi Rev 2, Nano Every
    // TCB1 is used by Tone()
    // TCB2 is used by Servo
    // TCB3 is used by millis()
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
    TCB0.CCMP = (F_CPU / IR_INTERRUPT_FREQUENCY) - 1;                   // compare value: 209 for 76 kHz, 221 for 72kHz @16MHz
    TCB0.INTCTRL = TCB_CAPT_bm;
    TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm;

#  else // __AVR_ATmega328__ here
    TCCR2A = _BV(WGM21); // CTC mode
#    if (F_CPU / IR_INTERRUPT_FREQUENCY) <= 256                         // for 8 bit timer
    TCCR2B = _BV(CS20); // no prescale
    OCR2B = OCR2A = (F_CPU / IR_INTERRUPT_FREQUENCY) - 1;// 209 for 76000 interrupts per second - toggle at each interrupt
#    else
    TCCR2B = _BV(CS21); // prescale by 8
    OCR2B = OCR2A = ((F_CPU / 8) / IR_INTERRUPT_FREQUENCY) - 1;// 132 for 15000 interrupts per second    TIFR2 = _BV(OCF2B) | _BV(OCF2A) | _BV(TOV2);// reset interrupt flags
#    endif
    TIMSK2 = _BV(OCIE2B);                                 // enable TIMER2_COMPB_vect interrupt to be compatible with tone() library
    TCNT2 = 0;
#  endif

#elif defined(ESP8266)
    timer1_isr_init();
    timer1_attachInterrupt(irmp_timer_ISR);
    /*
     * TIM_DIV1 = 0,   //80MHz (80 ticks/us - 104857.588 us max)
     * TIM_DIV16 = 1,  //5MHz (5 ticks/us - 1677721.4 us max)
     * TIM_DIV256 = 3 //312.5Khz (1 tick = 3.2us - 26843542.4 us max)
     */
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
    timer1_write((F_CPU / 16) / IR_INTERRUPT_FREQUENCY);

#elif defined(ESP32)
// Use Timer1 with 1 microsecond resolution, main clock is 80MHZ
    sESP32Timer = timerBegin(1, 80, true);
    timerAttachInterrupt(sESP32Timer, irmp_timer_ISR, true);
    timerAlarmWrite(sESP32Timer, (getApbFrequency() / 80) / IR_INTERRUPT_FREQUENCY, true);
    timerAlarmEnable(sESP32Timer);

#if defined(DEBUG) && defined(ESP32)
    Serial.print("CPU frequency=");
    Serial.print(getCpuFrequencyMhz());
    Serial.println("MHz");
    Serial.print("Timer clock frequency=");
    Serial.print(getApbFrequency());
    Serial.println("Hz");
#endif

// BluePill in 2 flavors
#elif defined(STM32F1xx) // stm32duino "Generic STM32F1 series" from STM32 Boards from STM32 cores of Arduino Board manager
    sSTM32Timer.setMode(LL_TIM_CHANNEL_CH1, TIMER_OUTPUT_COMPARE, NC);      // used for generating only interrupts, no pin specified
    sSTM32Timer.setPrescaleFactor(1);
    sSTM32Timer.setOverflow(F_CPU / IR_INTERRUPT_FREQUENCY, TICK_FORMAT);// microsecond period
//sSTM32Timer.setOverflow(1000000 / IR_INTERRUPT_FREQUENCY, MICROSEC_FORMAT); // microsecond period
    sSTM32Timer.attachInterrupt(irmp_timer_ISR);// this sets update interrupt enable
    sSTM32Timer.resume();// Start or resume HardwareTimer: all channels are resumed, interrupts are enabled if necessary

#elif defined(__STM32F1__) // for "Generic STM32F103C series" from STM32F1 Boards (Roger Clark's STM32duino.com) of manual installed hardware folder
    sSTM32Timer.setMode(TIMER_CH1, TIMER_OUTPUT_COMPARE);
    sSTM32Timer.setPrescaleFactor(1);
    sSTM32Timer.setOverflow(F_CPU / IR_INTERRUPT_FREQUENCY);
//sSTM32Timer.setPeriod(1000000 / IR_INTERRUPT_FREQUENCY);
    sSTM32Timer.attachInterrupt(TIMER_CH1, irmp_timer_ISR);
    sSTM32Timer.refresh();// Set the timer's count to 0 and update the prescaler and overflow values.

#elif defined(ARDUINO_ARCH_SAMD)
    REG_GCLK_CLKCTRL = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TCC2_TC3); // GCLK1=32kHz,  GCLK0=48MHz
//    while (GCLK->STATUS.bit.SYNCBUSY) // not required to wait
//        ;

    TcCount16* TC = (TcCount16*) TC3;

    TC->CTRLA.reg &= ~TC_CTRLA_ENABLE;// Enable write access to CTRLA register
    while (TC->STATUS.bit.SYNCBUSY == 1);// wait for sync

// Set Timer counter Mode to 16 bits, use match mode so that the timer counter resets when the count matches the compare register
    TC->CTRLA.reg |= TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_MFRQ |TC_CTRLA_PRESCALER_DIV1;

    TC->CC[0].reg = (uint16_t) ((F_CPU / IR_INTERRUPT_FREQUENCY)- 1);// ((48MHz / sampleRate) - 1);

// Enable the compare interrupt
    TC->INTENSET.reg = 0;
    TC->INTENSET.bit.MC0 = 1;

    NVIC_EnableIRQ (TC3_IRQn);

    TC->CTRLA.reg |= TC_CTRLA_ENABLE;
//    while (TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY); // Not required to wait at end of function

#elif defined(ARDUINO_ARCH_APOLLO3)
// Use Timer 3 segment B
    am_hal_ctimer_clear(3, AM_HAL_CTIMER_TIMERB);// reset timer
// only AM_HAL_CTIMER_FN_REPEAT resets counter after match (CTC mode)
    am_hal_ctimer_config_single(3, AM_HAL_CTIMER_TIMERB, (AM_HAL_CTIMER_INT_ENABLE | AM_HAL_CTIMER_HFRC_12MHZ | AM_HAL_CTIMER_FN_REPEAT));
    am_hal_ctimer_compare_set(3, AM_HAL_CTIMER_TIMERB, 0, 12000000 / IR_INTERRUPT_FREQUENCY);
    am_hal_ctimer_start(3, AM_HAL_CTIMER_TIMERB);

    am_hal_ctimer_int_register(AM_HAL_CTIMER_INT_TIMERB3, irmp_timer_ISR);
    am_hal_ctimer_int_enable(AM_HAL_CTIMER_INT_TIMERB3);
    NVIC_EnableIRQ(CTIMER_IRQn);
#endif
}

#if defined(_IRSND_H_) // we compile for irsnd
void storeIRTimer(void)
{
#  if defined(__AVR_ATmega16__)
    sTimerTCCRA = TCCR2;
    sTimerOCR = OCR2;
    sTimerTIMSK = TIMSK;

#  elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#    if defined(ARDUINO_AVR_DIGISPARK)
    sTimerTCCRA = TCCR0A;
    sTimerTCCRB = TCCR0B;
    sTimerOCRB = OCR0B;
    sTimerOCR = OCR0A;
    sTimerTIMSK = TIMSK;
#    else
    sTimerTCCRA = TCCR1;
    sTimerOCRB = OCR1B;
    sTimerOCR = OCR1C;
    sTimerTIMSK = TIMSK;
#    endif

#  elif defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
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

#  elif defined(__AVR__)
// store current timer state
    sTimerTCCRA = TCCR2A;
    sTimerTCCRB = TCCR2B;
    sTimerOCR = OCR2A;
    sTimerOCRB = OCR2B;
    sTimerTIMSK = TIMSK2;

#  elif defined(ESP8266)
    sTimerLoadValue= T1L;

#elif defined(ESP32)
    sTimerAlarmValue = timerAlarmRead(sESP32Timer);

#elif defined(STM32F1xx)
    sTimerOverflowValue = sSTM32Timer.getOverflow(TICK_FORMAT);

#elif defined(__STM32F1__)
    sTimerOverflowValue = sSTM32Timer.getOverflow();

#elif defined(ARDUINO_ARCH_SAMD)
    sTimerCompareCapureValue = TC3->COUNT16.CC[0].reg;

#elif defined(ARDUINO_ARCH_APOLLO3)
    sTimerCompareCapureValue = *((uint32_t *)CTIMERADDRn(CTIMER, 3, CMPRB0)) & 0xFFFF;
#  endif
}

/*
 * Restore settings of the timer e.g. for IRSND
 */
void restoreIRTimer(void)
{
#  if defined(__AVR_ATmega16__)
    TCCR2 = sTimerTCCRA;
    OCR2 = sTimerOCR;
    TIMSK = sTimerTIMSK;

#  elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#    if defined(ARDUINO_AVR_DIGISPARK)
    TCCR0A = sTimerTCCRA;
    TCCR0B = sTimerTCCRB;
    OCR0B = sTimerOCRB;
    OCR0A = sTimerOCR;
    TIMSK = sTimerTIMSK;
#    else
    TCCR1 = sTimerTCCRA;
    OCR1B = sTimerOCRB;
    OCR1C = sTimerOCR;
    TIMSK = sTimerTIMSK;
#    endif

#  elif  defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
    TCCR1B = sTimerTCCRB;
    ICR1 = sTimerOCR;
    OCR1B = sTimerOCRB;
    TIMSK1 = sTimerTIMSK;

#elif defined(__AVR_ATmega4809__) // Uno WiFi Rev 2, Nano Every
    TCB0.CTRLA = sTimerTCCRA;
    TCB0.CTRLB = sTimerTCCRB;
    TCB0.CCMP = sTimerOCR;
    TCB0.INTCTRL = sTimerTIMSK;

#  elif defined(__AVR__)
    TCCR2A = sTimerTCCRA;
    TCCR2B = sTimerTCCRB;
    OCR2A = sTimerOCR;
    OCR2B = sTimerOCRB;
    TIMSK2 = sTimerTIMSK;

#  elif defined(ESP8266)
    timer1_write(sTimerLoadValue);

#elif defined(ESP32)
    timerAlarmWrite(sESP32Timer, sTimerAlarmValue, true);

#elif defined(STM32F1xx)
    sSTM32Timer.setOverflow(sTimerOverflowValue, TICK_FORMAT);

#elif defined(__STM32F1__)
    sSTM32Timer.setOverflow(sTimerOverflowValue);

#elif defined(ARDUINO_ARCH_SAMD)
    TC3->COUNT16.CC[0].reg = sTimerCompareCapureValue;

#elif defined(ARDUINO_ARCH_APOLLO3)
    am_hal_ctimer_compare_set(3, AM_HAL_CTIMER_TIMERB, 0, sTimerCompareCapureValue);

#  endif
}
#endif // defined(_IRSND_H_)

#ifndef TIMER_INTERRUPT_EN_DISABLE_DEFINED
#define TIMER_INTERRUPT_EN_DISABLE_DEFINED
/*
 * NOT used if IRMP_ENABLE_PIN_CHANGE_INTERRUPT is defined
 * Initialize timer to generate interrupts at a rate F_INTERRUPTS (15000) per second to poll the input pin.
 */
void disableIRTimerInterrupt(void) {
#if defined(__AVR__)
// Use Timer 2
#  if defined(__AVR_ATmega16__)
    TIMSK = 0; // disable interrupt

#  elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#    if defined(ARDUINO_AVR_DIGISPARK)
    TIMSK &= ~_BV(OCIE0B); // disable interrupt
#    else
    TIMSK &= ~_BV(OCIE1B); // disable interrupt
#    endif

#  elif  defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
    TIMSK1 &= ~_BV(OCIE1A); // disable interrupt

#elif defined(__AVR_ATmega4809__) // Uno WiFi Rev 2, Nano Every
    TCB0.INTCTRL &= ~TCB_CAPT_bm;

#  else
    TIMSK2 = 0; // disable interrupt
#  endif // defined(__AVR_ATmega16__)

#elif defined(ESP8266)
    timer1_detachInterrupt(); // disables interrupt too

#elif defined(ESP32)
    timerAlarmDisable(sESP32Timer);

#elif defined(STM32F1xx)   // for "Generic STM32F1 series" from STM32 Boards from STM32 cores of Arduino Board manager
    sSTM32Timer.setMode(LL_TIM_CHANNEL_CH1, TIMER_DISABLED);
    sSTM32Timer.detachInterrupt();

#elif defined(__STM32F1__) // for "Generic STM32F103C series" from STM32F1 Boards (STM32duino.com) of manual installed hardware folder
    sSTM32Timer.setMode(TIMER_CH1, TIMER_DISABLED);
    sSTM32Timer.detachInterrupt(TIMER_CH1);

#elif defined(ARDUINO_ARCH_SAMD)
    TC3->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
//    while (TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY) ; // Not required to wait at end of function

#elif defined(ARDUINO_ARCH_APOLLO3)
    am_hal_ctimer_int_disable(AM_HAL_CTIMER_INT_TIMERB3);

#endif
}

void enableIRTimerInterrupt(void) {
#if defined(__AVR__)
// Use Timer 2
#  if defined(__AVR_ATmega16__)
    TIMSK = _BV(OCIE2); // enable interrupt

#  elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#    if defined(ARDUINO_AVR_DIGISPARK)
    TIMSK |= _BV(OCIE0B); // enable compare match interrupt
#    else
    TIMSK |= _BV(OCIE1B); // enable compare match interrupt
#    endif

#  elif  defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
    TIMSK1 |= _BV(OCIE1A); // enable compare match interrupt

#elif defined(__AVR_ATmega4809__) // Uno WiFi Rev 2, Nano Every
    TCB0.INTCTRL = TCB_CAPT_bm;

#  else
    TIMSK2 = _BV(OCIE2B); // enable interrupt
#  endif // defined(__AVR_ATmega16__)

#elif defined(ESP8266)
    timer1_attachInterrupt(irmp_timer_ISR); // enables interrupt too

#elif defined(ESP32)
    timerAlarmEnable(sESP32Timer);

#elif defined(STM32F1xx)   // for "Generic STM32F1 series" from STM32 Boards from STM32 cores of Arduino Board manager
    sSTM32Timer.setMode(LL_TIM_CHANNEL_CH1, TIMER_OUTPUT_COMPARE, NC); // used for generating only interrupts, no pin specified
    sSTM32Timer.attachInterrupt(irmp_timer_ISR);
    sSTM32Timer.refresh();// Set the timer's count to 0 and update the prescaler and overflow values.

#elif defined(__STM32F1__) // for "Generic STM32F103C series" from STM32F1 Boards (STM32duino.com) of manual installed hardware folder
    sSTM32Timer.setMode(TIMER_CH1, TIMER_OUTPUT_COMPARE);
    sSTM32Timer.attachInterrupt(TIMER_CH1, irmp_timer_ISR);
    sSTM32Timer.refresh(); // Set the timer's count to 0 and update the prescaler and overflow values.

#elif defined(ARDUINO_ARCH_SAMD)
    TC3->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
    while (TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY)
    ; //wait until TC5 is done syncing

#elif defined(ARDUINO_ARCH_APOLLO3)
    am_hal_ctimer_int_enable(AM_HAL_CTIMER_INT_TIMERB3);

#endif
}

#endif // TIMER_INTERRUPT_EN_DISABLE_DEFINED

/*
 * If both irmp and irsnd are used, compile it only once in the second step, when all variables are declared.
 */
#if ! defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND) || ( defined(IRMP_ARDUINO_EXT_H) && defined(IRSND_ARDUINO_EXT_H) )
/*
 * ISR is active while signal is sent AND during the trailing pause of IR frame
 * Called every 13.5us
 * Bit bang requires 5.9 us. 5 us for 16 push and 16 pop etc. and 0.9 us for function body
 * Together with call of irsnd_ISR() 10.5 us (frame) or 9.4 (trailing pause) - measured by scope
 * We use TIMER2_COMPB_vect to be compatible with tone() library
 */
#ifndef ISR_DEFINED
#define ISR_DEFINED

#if defined(ARDUINO_ARCH_APOLLO3)
/*
 * The isr, which calls the dispatcher/service, which call the irmp_timer_ISR
 */
extern "C" void am_ctimer_isr(void) {
    // Check and clear any active CTIMER interrupts.
    uint32_t ui32Status = am_hal_ctimer_int_status_get(true);
    am_hal_ctimer_int_clear(ui32Status);

    // Run handlers for the various possible timer events.
    am_hal_ctimer_int_service(ui32Status);
}
#endif

#if defined(__AVR__)

#  if F_CPU < 8000000L
#error "F_CPU must not be less than 8MHz for IRMP and IRSND"
#  endif

#  if defined(__AVR_ATmega16__)
ISR(TIMER2_COMP_vect)
#  elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
#    if defined(ARDUINO_AVR_DIGISPARK)
ISR(TIMER0_COMPB_vect)
#    else
ISR(TIMER1_COMPB_vect)
#    endif

#elif defined(__AVR_ATmega4809__) // Uno WiFi Rev 2, Nano Every
ISR(TCB0_INT_vect)

#  else
ISR(TIMER2_COMPB_vect)
#  endif // defined(__AVR_ATmega16__)

#elif defined(ESP8266)
// Required because irmp_ISR is declared as uint8_t
void ICACHE_RAM_ATTR irmp_timer_ISR(void)

#elif defined(ESP32)
void IRAM_ATTR irmp_timer_ISR(void)

#elif defined(ARDUINO_ARCH_SAMD)
void TC3_Handler(void)

#elif defined(STM32F1xx) && STM32_CORE_VERSION_MAJOR == 1 &&  STM32_CORE_VERSION_MINOR <= 8 // for "Generic STM32F1 series" from STM32 Boards from STM32 cores of Arduino Board manager
void irmp_timer_ISR(HardwareTimer * aDummy __attribute__((unused))) // changed in stm32duino 1.9 - 5/2020

#else // STM32F1xx (v1.9), __STM32F1__, ARDUINO_ARCH_APOLLO3
void irmp_timer_ISR(void)

#endif // defined(__AVR__)

// Start of ISR
{
#if defined(ARDUINO_ARCH_SAMD)
    TC3->COUNT16.INTFLAG.bit.MC0 = 1; // Clear interrupt

#elif defined(ARDUINO_ARCH_APOLLO3)
    // Clear interrupt
#endif

#  if defined(_IRSND_H_) || defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND)
    static uint8_t sDivider = 4; // IR signal toggle rate is 4 times IRSND call rate
#  endif

#  ifdef IRMP_MEASURE_TIMING
    digitalWriteFast(IRMP_TIMING_TEST_PIN, HIGH); // 2 clock cycles
#  endif

#  if defined(_IRSND_H_) || defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND)
    /*
     * Send part of ISR
     */
    if(irsnd_busy) {
        if (irsnd_is_on)
        {
#    if defined(digitalToggleFast)
#      if defined(IRMP_IRSND_ALLOW_DYNAMIC_PINS) && defined (__AVR__)
            *irsnd_output_pin_input_port |= irsnd_output_pin_mask; // fast toggle for AVR
#      else
            digitalToggleFast(IRSND_OUTPUT_PIN);
#      endif // defined(IRMP_IRSND_ALLOW_DYNAMIC_PINS)  && defined (__AVR__)
#    else
            digitalWrite(IRSND_OUTPUT_PIN, !digitalRead(IRSND_OUTPUT_PIN));
#    endif
        }

        if (--sDivider == 0)
        {
            // empty call needs additional 0.7 us
            if (!irsnd_ISR())
            {
                restoreIRTimer();
#    if ! defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND)
// only send active -> disable interrupt
                disableIRTimerInterrupt();
#    endif
            }
            sDivider = 4;
        }
    } // if(irsnd_busy)
#  endif // defined(_IRSND_H_) || defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND)
#  if defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND)
    else
    { // for receive and send in one ISR
#  endif

#  if defined(_IRMP_H_) || defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND)
        /*
         * Receive part of ISR
         */
        irmp_ISR();
#  endif

#  if defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND)
    } // for receive and send in one ISR
#  endif

#  ifdef IRMP_MEASURE_TIMING
    digitalWriteFast(IRMP_TIMING_TEST_PIN, LOW); // 2 clock cycles
#  endif
}
#endif // ISR_DEFINED
#endif // ! defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND) || ( defined(IRMP_ARDUINO_EXT_H) && defined(IRSND_ARDUINO_EXT_H) )
