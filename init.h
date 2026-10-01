/*
 * init.h
 *
 *  Created on: Sep 8, 2026
 *      Author: david
 */

#ifndef INIT_H_
#define INIT_H_

#include <stdint.h> /* Essential for uint8_t and uint16_t definitions */

void initClocks(void);
void initGPIO(void);
void init_spi_shift_register(void);
void initSideToneTimer(void);
void initQSKTimer(uint16_t);
void initKeyTimer(uint8_t);

#define MENU_ENCODER_A GPIO_PORT_P5, GPIO_PIN1
#define MENU_ENCODER_B GPIO_PORT_P5, GPIO_PIN2
#define MENU_ENCODER_SWITCH GPIO_PORT_P5, GPIO_PIN0

#define LED_ENCODER_A GPIO_PORT_P4, GPIO_PIN7
#define LED_ENCODER_B GPIO_PORT_P6, GPIO_PIN2
#define LED_ENCODER_SWITCH GPIO_PORT_P6, GPIO_PIN1

#define TUNER_ENCODER_A GPIO_PORT_P4, GPIO_PIN5
#define TUNER_ENCODER_B GPIO_PORT_P2, GPIO_PIN2
#define TUNER_ENCODER_SWITCH GPIO_PORT_P4, GPIO_PIN6
/*
 * temporary location for defines of gpio ports
 * move to individual *.h files when the *.c files are created
 */
#define POWER_DRV_ENABLE GPIO_PORT_P1, GPIO_PIN4
#define FILTER_SELECT GPIO_PORT_P5, GPIO_PIN4
#define VDD_SENSE GPIO_PORT_P1, GPIO_PIN0
#define SIDETONE_OUTPUT GPIO_PORT_P1, GPIO_PIN1
#define REG_CLK  GPIO_PORT_P5, GPIO_PIN5
#define SELECT_BINAURAL  GPIO_PORT_P5, GPIO_PIN6
#define BTN_TXMODE  GPIO_PORT_P5, GPIO_PIN7
#define TX_LED  GPIO_PORT_P6, GPIO_PIN0
#define POWER_AMP_ENABLE  GPIO_PORT_P3, GPIO_PIN0
#define SPARE_P3P3  GPIO_PORT_P3, GPIO_PIN3
#define SPARE_P2P3  GPIO_PORT_P2, GPIO_PIN3
#define SPARE_P3P4  GPIO_PORT_P3, GPIO_PIN4
#define SPARE_P3P1  GPIO_PORT_P3, GPIO_PIN1
#define SPARE_P2P5  GPIO_PORT_P2, GPIO_PIN5
#define SPARE_P3P7  GPIO_PORT_P3, GPIO_PIN7
#define POWER_RF_ENABLE  GPIO_PORT_P4, GPIO_PIN0

/*
 * Note: the convention is that the left paddle (normal dits) should connect to
 * the tip, and the right paddle (normal dahs) should connect to the ring.  My HW
 * has this reversed, so below swap the mapping to correct this HW connection.
 */
#define DAH_KEY  GPIO_PORT_P4, GPIO_PIN1
#define DIT_KEY  GPIO_PORT_P4, GPIO_PIN2

#define STRAIGHT_KEY  GPIO_PORT_P2, GPIO_PIN7
#define TR_SWITCH  GPIO_PORT_P3, GPIO_PIN5
#define CWTX_OUT  GPIO_PORT_P3, GPIO_PIN2
#define MUTE_OUT  GPIO_PORT_P3, GPIO_PIN6


#endif /* INIT_H_ */
