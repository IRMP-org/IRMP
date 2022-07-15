/*
 * irmpVersion.h
 *
 * Is used for IRMP and IRSND, and therefore a separate file
 *
 *  Created on: 28.05.2020
 *      Author: Armin
 */

#ifndef SRC_LIB_IRMPVERSION_H_
#define SRC_LIB_IRMPVERSION_H_

#define VERSION_IRMP "3.6.1"
#define VERSION_IRMP_MAJOR 3
#define VERSION_IRMP_MINOR 6
#define VERSION_IRMP_PATCH 0

/*
 * Macro to convert 3 version parts into an integer
 * To be used in preprocessor comparisons, such as #if VERSION_IRMP_HEX >= VERSION_HEX_VALUE(3, 7, 0)
 */
#define VERSION_HEX_VALUE(major, minor, patch) ((major << 16) | (minor << 8) | (patch))
#define VERSION_IRMP_HEX  VERSION_HEX_VALUE(VERSION_IRMP_MAJOR, VERSION_IRMP_MINOR, VERSION_IRMP_PATCH)
#endif /* SRC_LIB_IRMPVERSION_H_ */
