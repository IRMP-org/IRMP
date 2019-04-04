
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

# Schematic for Arduino UNO
![Fritzing schematic for Arduino UNO](https://github.com/ukw100/IRMP/blob/master/extras/IRMP_UNO_Steckplatine.png)

# Quick comparison of 4 Arduino IR receiving libraries
## This is a short comparison and may not be complete or correct
[I](ArminJo) did it for myself in order to choose a small IR lib for my project and to have a quick overview, when to choose which library.
It is dated **2.4.2019**. If you have complains about the data or request for extensions, please send a PM or open an issue.


| Subject | [IRMP](https://github.com/ukw100/IRMP) | [IRLremote](https://github.com/NicoHood/IRLremote) | [IRLib2](https://github.com/cyborg5/IRLib2) | [IRremote](https://github.com/z3t0/Arduino-IRremote) |
|---------|------|-----------|--------|----------|
| Number of protocols | 50 | 2 + Hash \* | 12 + Hash \* | 13 |
| 3.Party libs needed| % | PinChangeInterrupt if not pin 2 or 3 | % | % |
| Timing method | Timer2 or interrupt for pin 2 or 3 | Interrupt | Timer2 or interrupt for pin 2 or 3 | Timer2 |
| Decode method | OnTheFly | OnTheFly | RAM | RAM |
| FLASH usage (simple NEC example with 5 prints) | 1500<br/>(4300 for 15 main / 8000 for all 42 protocols)<br/>(+200 for callback)<br/>(+80 for interrupt at pin 2+3)| 1270<br/>(1400 for pin 2+3) | 4830 | 3210 |
| RAM usage | 52<br/>(73 / 100 for 15 (main) / 42 protocols) | 62 | 334 | 227 |
| Supported platforms | avr<br/>(and avr, esp8266, arm, pic for non Arduino IDE) | avr, esp8266 | avr, arm(samd) | avr, attiny, esp8266, esp32, arm |
| Last library update | 4/2019 | 3/2018 | 6/2018 | 11/2017 |
| Remarks | Decodes 42 protocols concurrently.<br/>Work in progress. | Only one protocol at a time. | LED 13 Feedback. | LED 13 Feedback.<br/>A lot of branches available and an active community |

\*The Hash protocol gives you a hash as code, which may be sufficient to distinguish your keys on the remote, but may not work with some protocols like Mitsubishi
