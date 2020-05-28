/*
 * IRTimer.h
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
#ifndef IRTIMER_H
#define IRTIMER_H

#if defined(ARDUINO)
#if ! defined(USE_ONE_TIMER_FOR_IRMP_AND_IRSND) && defined(IRMP_H) && defined(IRSND_H)
#error "You seem to use receive and send in one sketch but forget to define USE_ONE_TIMER_FOR_IRMP_AND_IRSND. Unfortunately this cannot be done automatically."
#endif

#if defined(IRMP_H)
extern void     initIRReceiveTimer(void);
#else
extern void     IRInitSendTimer(void);
#endif

//#if defined(STM32F1xx)   // for "Generic STM32F1 series" from STM32 Boards from STM32 cores of Arduino Board manager
//extern void     irmp_timer_ISR(HardwareTimer * aDummy __attribute__((unused))); // changed in stm32duino 1.9
//#else
extern void     irmp_timer_ISR(void);
//#endif

extern void     disableIRTimerInterrupt(void);
extern void     enableIRTimerInterrupt(void);

extern void     storeIRTimer(void);
extern void     restoreIRTimer(void);
#endif // defined(ARDUINO)

#endif // IRTIMER_CPP_H

#pragma once
