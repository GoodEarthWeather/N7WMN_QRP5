/*
 * si5351.h
 *
 *  Created on: Sep 26, 2026
 *      Author: david
 */

#ifndef SI5351_H_
#define SI5351_H_

int setSI5351Freq(uint32_t);
void initialize_si5351(void);
void si5351_disable_spread_spectrum(void);


#endif /* SI5351_H_ */
