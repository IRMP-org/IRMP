/*
 * irmpVersion.h
 *
 * Is used for IRMP and IRSND, and therefore a separate file
 *
 *  Created on: 28.05.2020
 *      Author: Armin
 */

#ifndef _IRMPVERSION_H
#define _IRMPVERSION_H

#define VERSION_IRMP "3.6.4"
#define VERSION_IRMP_MAJOR 3
#define VERSION_IRMP_MINOR 6
#define VERSION_IRMP_PATCH 4

/*
 * Macro to convert 3 version parts into an integer
 * To be used in preprocessor comparisons, such as #if VERSION_IRMP_HEX >= VERSION_HEX_VALUE(3, 7, 0)
 */
#define VERSION_HEX_VALUE(major, minor, patch) ((major << 16) | (minor << 8) | (patch))
#define VERSION_IRMP_HEX  VERSION_HEX_VALUE(VERSION_IRMP_MAJOR, VERSION_IRMP_MINOR, VERSION_IRMP_PATCH)
#endif /* _IRMPVERSION_H */
