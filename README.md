
# IRMP - Infrared Multi Protocol Decoder
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Build Status](https://travis-ci.org/ArminJo/irmp.svg?branch=master)](https://travis-ci.org/ArminJo/irmp)

## 50 IR protocols supported and low memory footprint
not all protocols can be enabled at the same time, since some of them are quite similar

# Documentation at mikrocontroller.net
### English
   http://www.mikrocontroller.net/articles/IRMP_-_english
### German
   http://www.mikrocontroller.net/articles/IRMP
   
# List of protocols
- Sony SIRCS, NEC + APPLE, Samsung + Samsg32, Kaseikyo
- JVC, NEC16, NEC42, Matsushita, DENON, Sharp, RC5, RC6 & RC6A, IR60 (SDA2008) Grundig, Siemens Gigaset, Nokia
- BOSE, Kathrein , NUBERT , FAN (ventilator) , SPEAKER (~NUBERT), Bang & Olufsen , RECS80 (SAA3004) , RECS80EXT (SAA3008), Thomson, NIKON camera , Netbox keyboard, ORTEK (Hama) , Telefunken 1560, FDC3402 keyboard , RC Car , iRobot Roomba, RUWIDO, T-Home , A1 TV BOX, LEGO Power RC, RCMM 12,24, or 32, LG Air Condition , Samsung48, Merlin , Pentax , S100 , ACP24, TECHNICS , PANASONIC Beamer , Mitsubishi Aircond , VINCENT, SAMSUNG AH , IRMP specific, GREE CLIMATE , RCII T+A, RADIO e.g. TEVION

# Schematic for Arduino UNO
![Fritzing schematic for Arduino UNO](https://github.com/ukw100/IRMP/blob/master/extras/IRMP_UNO_Steckplatine.png)

# AllProtocol example
![Nano running AllProtocol example](https://github.com/ukw100/IRMP/blob/master/extras/NEC.jpg)

# Quick comparison of 4 Arduino IR receiving libraries
## This is a short comparison and may not be complete or correct
[I](ArminJo) did it for myself in order to choose a small IR lib for my project and to have a quick overview, when to choose which library.
It is dated **2.4.2019**. If you have complains about the data or request for extensions, please send a PM or open an issue.


| Subject | [IRMP](https://github.com/ukw100/IRMP) | [IRLremote](https://github.com/NicoHood/IRLremote) | [IRLib2](https://github.com/cyborg5/IRLib2) | [IRremote](https://github.com/z3t0/Arduino-IRremote) |
|---------|------|-----------|--------|----------|
| Number of protocols | **50** | 2 + Hash \* | 12 + Hash \* | 13 |
| 3.Party libs needed| % | PinChangeInterrupt if not pin 2 or 3 | % | % |
| Timing method | Timer2 or interrupt for pin 2 or 3 | **Interrupt** | Timer2 or interrupt for pin 2 or 3 | Timer2 |
| Decode method | OnTheFly | OnTheFly | RAM | RAM |
| FLASH usage (simple NEC example with 5 prints) | 1500<br/>(4300 for 15 main / 8000 for all 42 protocols)<br/>(+200 for callback)<br/>(+80 for interrupt at pin 2+3)| **1270**<br/>(1400 for pin 2+3) | 4830 | 3210 |
| RAM usage | **52**<br/>(73 / 100 for 15 (main) / 42 protocols) | **62** | 334 | 227 |
| Supported platforms | **avr, esp8266<br/>(plus arm and pic for non Arduino IDE)** | avr, esp8266 | avr, arm(samd) | avr, attiny, esp8266, esp32, arm |
| Last library update | 4/2019 | 3/2018 | 6/2018 | 11/2017 |
| Remarks | LED 13 Feedback.<br/>Decodes 42 protocols concurrently.<br/>Work in progress. | Only one protocol at a time. | LED 13 Feedback. | LED 13 Feedback.<br/>NEC decoding is poor.<br/>**A lot of branches available and there is an active community** |

\*The Hash protocol gives you a hash as code, which may be sufficient to distinguish your keys on the remote, but may not work with some protocols like Mitsubishi

# Easy migrating your code from IRremote to IRMP
See also the [SimpleReceiver example](https://github.com/ukw100/IRMP/blob/master/examples/SimpleReceiver/SimpleReceiver.ino) .

### Change the include and declarations from:
```
#include <IRremote.h>
#define IR_RECEIVER_PIN 3
IRrecv myReceiver(IR_RECEIVER_PIN);
decode_results results;
```
to
```
#define IRMP_INPUT_PIN 3
#define IRMP_PROTOCOL_NAMES 1
#include <irmpMain15.h>
#include <irmp.c.h>
IRMP_DATA irmp_data[1];
```

### In setup change:
`myReceiver.enableIRIn();` to `irmp_init();`

### To get the data, change:
`if (myReceiver.decode(&results))` to `if (irmp_get_data(&irmp_data[0]))`<br/>
and<br/>
`switch (results.value)` to `switch (irmp_data[0].command)`.

You do not need **`myReceiver.resume();`** any more, just delete it.

The code representation also often changes. In IRMP (as in IRLremote) it is more standard and simpler but different. Use the code from the [SimpleReceiver example](https://github.com/ukw100/IRMP/blob/master/examples/SimpleReceiver/SimpleReceiver.ino) to print the new code representation.

If you want to distinguish between more than one remote you may also use `irmp_data[0].address` like it is done in the [Callback example](https://github.com/ukw100/IRMP/blob/master/examples/Callback/Callback.ino).
