/*
 * isr.c
 *
 *  Created on: Sep 5, 2026
 *      Author: david
 */
#include "driverlib.h"
#include "isr.h"
#include "init.h"

uint8_t volatile buttonPressed = BTN_PRESSED_NONE;
uint8_t encoderCWCount, encoderCCWCount;

// Port 3 interrupt service routine
#pragma vector=PORT3_VECTOR
__interrupt void Port_3(void)
{
    switch(__even_in_range(P3IV,P3IV_P3IFG7))
    {
      case  P3IV_P3IFG2:
          break;
      default: break;
    }
}


// Port 2 interrupt service routine
#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void)
{
    switch(__even_in_range(P2IV,P2IV_P2IFG7))
    {
      case  P2IV_P2IFG7:
          break;
      default: break;
    }
}
// Port 4 interrupt service routine
#pragma vector=PORT4_VECTOR
__interrupt void Port_4(void)
{
    switch(__even_in_range(P4IV,P4IV_P4IFG7))
    {
    case  P4IV_P4IFG1:
        // P4.1
        buttonPressed = BTN_PRESSED_PADDLE_RIGHT;
        break;
    case  P4IV_P4IFG2:
        // P4.2
        buttonPressed = BTN_PRESSED_PADDLE_LEFT;
        break;
    case  P4IV_P4IFG5:
        // P4.5 = tuner encoder A
        buttonPressed = BTN_PRESSED_TUNER_ENCODER;
        if (GPIO_getInputPinValue(TUNER_ENCODER_A) != GPIO_getInputPinValue(TUNER_ENCODER_B))
        {
            encoderCWCount++;
        } else {
            encoderCCWCount++;
        }
        // now toggle interrupt edge
        P4IES ^= BIT5;
        break;
    case  P4IV_P4IFG6:
        // P4.6 = tuner encoder switch
        buttonPressed = BTN_PRESSED_TUNER_ENCODER_SWITCH;
        break;
      case  P4IV_P4IFG7:
          // P4.7 = LED encoder A
          buttonPressed = BTN_PRESSED_LED_ENCODER;
          break;

      default: break;
    }
}

// Port 5 interrupt service routine
#pragma vector=PORT5_VECTOR
__interrupt void Port_5(void)
{
    switch(__even_in_range(P5IV,P5IV_P5IFG7))
    {
    case P5IV_P5IFG0:
        // P5.0 = MENU encoder switch
        buttonPressed = BTN_PRESSED_MENU_ENCODER_SWITCH;
        break;
      case  P5IV_P5IFG1:
          // P5.1 = Menu Encoder A
          buttonPressed = BTN_PRESSED_MENU_ENCODER;
          break;
      case  P5IV_P5IFG7:
          // P5.7 = TX Mode Button
          buttonPressed = BTN_PRESSED_TX_MODE;
          break;

      default: break;
    }
}

// Port 6 interrupt service routine
#pragma vector=PORT6_VECTOR
__interrupt void Port_6(void)
{
    switch(__even_in_range(P6IV,P6IV_P6IFG7))
    {
    case P6IV_P6IFG1:
        // P6.1 = LED encoder switch
        buttonPressed = BTN_PRESSED_LED_ENCODER_SWITCH;
        break;
      default: break;
    }
}

