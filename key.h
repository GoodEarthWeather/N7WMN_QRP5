/*
 * key.h
 *
 *  Created on: Sep 30, 2026
 *      Author: david
 */

#ifndef KEY_H_
#define KEY_H_

#define DIT 0x1
#define DAH 0x3

#define TX_KEY_UP 0x0
#define TX_KEY_DOWN 0x1

#define NORMAL 0

void ditdah(uint8_t);
void playCwMsg(uint8_t);
void recordCwMsg(uint8_t);

#endif /* KEY_H_ */
