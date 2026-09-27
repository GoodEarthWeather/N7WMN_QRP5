
#ifndef LCDLIB_H_
#define LCDLIB_H_

#include "driverlib.h"
#include "menu_data.h"
#include <string.h>

typedef struct
{
    uint8_t baseAddr;    /* DDRAM address, e.g. 0x00 for FREQ, 0x40 for STATUS */
    uint8_t fieldWidth;  /* characters, e.g. 12 for STATUS */
} LcdField_t;

// Functions
void lcdInit();                                 // Initialize LCD
void moveFreqCursor(uint8_t);
void updateLCD_status(void);
void LCD_WriteField(const LcdField_t *, const char *, const char *);
void updateLCD_freq(void);

// Delay Functions
// Modified for an 8MHz clock
#define delay_ms(x)		__delay_cycles((long) x* 1000 * 8)
#define delay_us(x)		__delay_cycles((long) x * 8)

// Pins

#define MOVE_CURSOR 0x80   // add address to this for move cursor command
#define LCD_D4 GPIO_PORT_P1, GPIO_PIN7
#define LCD_D5 GPIO_PORT_P4, GPIO_PIN3
#define LCD_D6 GPIO_PORT_P4, GPIO_PIN4
#define LCD_D7 GPIO_PORT_P5, GPIO_PIN3
#define LCD_RS GPIO_PORT_P1, GPIO_PIN5
#define LCD_CLK GPIO_PORT_P1, GPIO_PIN6





#endif /* LCDLIB_H_ */
