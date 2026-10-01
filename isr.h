/*
 * isr.h
 *
 *  Created on: Sep 8, 2026
 *      Author: david
 */

#ifndef ISR_H_
#define ISR_H_

extern uint8_t volatile buttonPressed;

// Define buttons
#define BTN_PRESSED_NONE 0x0
#define BTN_PRESSED_MENU_ENCODER 0x1
#define BTN_PRESSED_LED_ENCODER 0x2
#define BTN_PRESSED_TUNER_ENCODER 0x3
#define BTN_PRESSED_MENU_ENCODER_SWITCH 0x4
#define BTN_PRESSED_LED_ENCODER_SWITCH 0x5
#define BTN_PRESSED_TUNER_ENCODER_SWITCH 0x6
#define BTN_PRESSED_DIT 0x7
#define BTN_PRESSED_DAH 0x8

#define ENCODER_CW   1
#define ENCODER_CCW  -1

#endif /* ISR_H_ */
