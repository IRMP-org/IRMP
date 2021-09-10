/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * irmp.hpp - infrared multi-protocol decoder, supports several remote control protocols
 *
 * Copyright (c) 2009-2019 Frank Meyer - frank(at)fli4l.de
 * 2020 Arduino porting by Armin Joachimsmeyer
 *
 * Supported AVR mikrocontrollers:
 *
 * ATtiny87,  ATtiny167
 * ATtiny45,  ATtiny85
 * ATtiny44,  ATtiny84
 * ATmega8,   ATmega16,  ATmega32
 * ATmega162
 * ATmega164, ATmega324, ATmega644,  ATmega644P, ATmega1284, ATmega1284P
 * ATmega88,  ATmega88P, ATmega168,  ATmega168P, ATmega328P
 *
 * This file is part of IRMP https://github.com/ukw100/IRMP.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

#error  ------For version >= 3.5.0 you must change: #include <irmp.c.h> to: #include <irmp.hpp>------
