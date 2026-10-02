/*
 * key.c
 *
 *  Created on: Feb 23, 2022
 *      Author: fishi
 */

#include "driverlib.h"
#include "main.h"
#include "lcdLib.h"
#include "radio_state.h"
#include "key.h"
#include "init.h"
#include "isr.h"

static void keyDown(void);
static void keyUp(void);

#define NORMAL 0
#define ENABLED 1
#define DISABLED 0
#define RECORD 1
#define PLAY 2
#define IAMBIC_MODE_A 0
#define IAMBIC_MODE_B 1
#define IAMBIC_MODE_ULTIMATIC 2

#define CW_MSG_COUNT 3  // number of messages
#define CW_MEM_SIZE 512  // length of vector
#define CWMSG_DISABLED 0
#define CWMSG_PLAY 1
#define CWMSG_RECORD 2


typedef struct {
    uint16_t mem[CW_MEM_SIZE];
} cwMem_t;

#pragma PERSISTENT(cwMsg)
static cwMem_t cwMsg[CW_MSG_COUNT] = {0};  // vector to hold cw message

static uint16_t *dataPtr;
static uint8_t cwMsgState;


// This routine will start transmission
static void keyDown(void)
{

    muteAudio();
    // stop qsk timer
    Timer_A_stop(TIMER_A1_BASE);
    Timer_A_clearCaptureCompareInterrupt(TIMER_A1_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);
    Timer_A_disableCaptureCompareInterrupt(TIMER_A1_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);
    //setTRSwitch(TRANSMIT);

    radioState.txKeyState = TX_KEY_DOWN;
    //si5351_RXTX_enable();
    GPIO_setOutputHighOnPin(CWTX_OUT);

}

// This routine will stop transmission
static void keyUp(void)
{

    Timer_A_clearCaptureCompareInterrupt(TIMER_A1_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);
    Timer_A_clear(TIMER_A1_BASE);  // reset QSK timer
    Timer_A_enableCaptureCompareInterrupt(TIMER_A1_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);
    Timer_A_startCounter( TIMER_A1_BASE,TIMER_A_CONTINUOUS_MODE);
    radioState.txKeyState = TX_KEY_UP;
    GPIO_setOutputLowOnPin(CWTX_OUT);
    delay_ms(10);
    //setTRSwitch(RECEIVE);
    //si5351_RXTX_enable();
}

/*****************
// This routine will turn on the transmitter for tuning
void setTuneMode(void)
{
    extern uint8_t tuneMode;
    extern uint8_t txKeyState;
    extern uint8_t txMode;

    if (txMode == ENABLED)  // tune mode only if in txmode
    {
        if (tuneMode == ENABLED)
        {
            keyDown();
            Timer_A_startCounter(TIMER_A0_BASE,TIMER_A_UP_MODE);  // start side tone
        }
        else
        {
            keyUp();
            Timer_A_stop(TIMER_A0_BASE);  // stop side tone
            Timer_A_setOutputForOutputModeOutBitValue(TIMER_A0_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_1,TIMER_A_OUTPUTMODE_OUTBITVALUE_LOW);
        }
    }
}
*****************/

/************************
// This routine will set the state of the tr switch
void setTRSwitch(uint8_t state)
{
    if (state == RECEIVE)
        GPIO_setOutputHighOnPin(TR_SWITCH); // receive mode
    else if (state == TRANSMIT)
        GPIO_setOutputLowOnPin(TR_SWITCH); // transmit mode
}
******************/


// This routine will play the cw message in the cwMsg[] vector
void playCwMsg(uint8_t mem)
{
    uint16_t count;
    uint8_t done;
    uint8_t on = 0;

    cwMsgState = CWMSG_PLAY;

    dataPtr = cwMsg[mem].mem;
    buttonPressed = BTN_PRESSED_NONE;
    dataPtr++;  // skip the 0 in the first location

    // configure message timer (A3)
    initCWMsgPlayTimer();
    on = 0;
    while ( (count = *dataPtr++) && (buttonPressed == BTN_PRESSED_NONE) )
    {
        on = !on;  // switch to on or off
        Timer_A_clear(TIMER_A3_BASE);  // clear timer
        Timer_A_setCompareValue (TIMER_A3_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0, count);
        if (on)
        {
            Timer_A_startCounter(TIMER_A0_BASE,TIMER_A_UP_MODE);  // start side tone
            if (radioState.txMode)
                keyDown();
        }
        Timer_A_startCounter(TIMER_A3_BASE,TIMER_A_UP_MODE);
        do {
            done = Timer_A_getCaptureCompareInterruptStatus(TIMER_A3_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0,TIMER_A_CAPTURECOMPARE_INTERRUPT_FLAG);
        } while (done != TIMER_A_CAPTURECOMPARE_INTERRUPT_FLAG);
        Timer_A_clearCaptureCompareInterrupt(TIMER_A3_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
        if (on)
        {
            Timer_A_stop(TIMER_A0_BASE);  // stop side tone
            if (radioState.txMode)
                keyUp();
        }
    }
    buttonPressed = BTN_PRESSED_NONE;  // if playback was interrupted by keypress, ignore the key that was pressed
    cwMsgState = CWMSG_DISABLED;
}

void recordCwMsg(uint8_t mem)
{

    cwMsgState = CWMSG_RECORD;
    dataPtr = cwMsg[mem].mem;
    buttonPressed = BTN_PRESSED_NONE;
    initCWMsgRecordTimer();
    while ( (buttonPressed != BTN_PRESSED_MENU_ENCODER_SWITCH) )
    {
        switch (buttonPressed)
        {
        case BTN_PRESSED_PADDLE_LEFT :
            buttonPressed = BTN_PRESSED_NONE;
            (radioState.paddleOrientation == NORMAL) ? ditdah(DIT) : ditdah(DAH);
            break;
        case BTN_PRESSED_PADDLE_RIGHT :
            buttonPressed = BTN_PRESSED_NONE;
            (radioState.paddleOrientation == NORMAL) ? ditdah(DAH) : ditdah(DIT);
            break;
        default :
            break;
        }
    }
    *dataPtr = 0; // put a zero at the end of the message
    cwMsgState = CWMSG_DISABLED;

    buttonPressed = BTN_PRESSED_NONE;
}

// This routine is to handle the dit and dah key
// 'key' is either DIT or DAH (i.e. 1 or 3)
void ditdah(uint8_t key)
{
    uint8_t done;
    uint8_t count;
    uint8_t ditKeyState;
    uint8_t dahKeyState;
    uint8_t squeeze = DISABLED;

    do {

        // start the sidetone and, if needed, cw message timer
        Timer_A_startCounter(TIMER_A0_BASE,TIMER_A_UP_MODE);  // start side tone

        if (cwMsgState == CWMSG_RECORD)
        {
            Timer_A_stop(TIMER_A3_BASE);  // stop cw msg timer
            if (Timer_A_getInterruptStatus(TIMER_A3_BASE) == TIMER_A_INTERRUPT_PENDING)
            {
                // timer overflow - limit delay to max (2 seconds)
                *dataPtr++ = 0xFFFF;
                Timer_A_clearTimerInterrupt(TIMER_A3_BASE);
            } else {
                *dataPtr++ = Timer_A_getCounterValue(TIMER_A3_BASE);
            }
            Timer_A_clear(TIMER_A3_BASE);  // clear timer
            Timer_A_startCounter(TIMER_A3_BASE,TIMER_A_CONTINUOUS_MODE);  // start measuring keyDown time
        }

        // if transmitter enabled, turn on
        if (radioState.txMode)
            keyDown();

        // Prepare dit dah timer
        Timer_A_clearCaptureCompareInterrupt(TIMER_A2_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
        Timer_A_clear(TIMER_A2_BASE);  // clear timer

        // Start dit/dah delay timer
        count = 0;
        while (count < key)
        {
            done = Timer_A_getCaptureCompareInterruptStatus(TIMER_A2_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0,TIMER_A_CAPTURECOMPARE_INTERRUPT_FLAG);
            if (done == TIMER_A_CAPTURECOMPARE_INTERRUPT_FLAG)
            {
                count++;
                Timer_A_clearCaptureCompareInterrupt(TIMER_A2_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
            }
        }
        //  stop transmission (if enabled) and stop side tone and cw message timer if needed
        if (radioState.txMode)
            keyUp();
        Timer_A_stop(TIMER_A0_BASE);  // stop side tone

        if (cwMsgState == CWMSG_RECORD)
        {
            Timer_A_stop(TIMER_A3_BASE);  // stop cw msg timer
            *dataPtr++ = Timer_A_getCounterValue(TIMER_A3_BASE);
            Timer_A_clear(TIMER_A3_BASE);  // clear timer
            Timer_A_startCounter(TIMER_A3_BASE,TIMER_A_CONTINUOUS_MODE);  // start measuring keyUp time
        }

        // Wait one dit time
        Timer_A_clear(TIMER_A2_BASE);  // clear dit/dah timer
        Timer_A_clearCaptureCompareInterrupt(TIMER_A2_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
        do {
            done = Timer_A_getCaptureCompareInterruptStatus(TIMER_A2_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0,TIMER_A_CAPTURECOMPARE_INTERRUPT_FLAG);
        } while (done != TIMER_A_CAPTURECOMPARE_INTERRUPT_FLAG);
        Timer_A_clearCaptureCompareInterrupt(TIMER_A2_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);

        // completion of key sent; now determine what to do next
        if (radioState.paddleOrientation == NORMAL){
            ditKeyState = GPIO_getInputPinValue(PADDLE_LEFT);
            dahKeyState = GPIO_getInputPinValue(PADDLE_RIGHT);
        }
        else {
            ditKeyState = GPIO_getInputPinValue(PADDLE_RIGHT);
            dahKeyState = GPIO_getInputPinValue(PADDLE_LEFT);
        }

        if (ditKeyState == GPIO_INPUT_PIN_LOW && dahKeyState == GPIO_INPUT_PIN_LOW){
            squeeze = ENABLED;
            if (radioState.keyerIndex != IAMBIC_MODE_ULTIMATIC){
                (key == DAH) ? (key = DIT) : (key = DAH);  // alternate dit/dah
            } else if (buttonPressed != BTN_PRESSED_NONE){
                if (radioState.paddleOrientation == NORMAL)
                    (buttonPressed == BTN_PRESSED_PADDLE_LEFT) ? (key = DIT) : (key = DAH);
                else
                    (buttonPressed == BTN_PRESSED_PADDLE_LEFT) ? (key = DAH) : (key = DIT);
            }
        } else if (ditKeyState == GPIO_INPUT_PIN_LOW && dahKeyState == GPIO_INPUT_PIN_HIGH) {
            squeeze = DISABLED;
            key = DIT;
        } else if (ditKeyState == GPIO_INPUT_PIN_HIGH && dahKeyState == GPIO_INPUT_PIN_LOW) {
            squeeze = DISABLED;
            key = DAH;
        } else  {  // both keys are high
            if (radioState.paddleOrientation == NORMAL && buttonPressed == BTN_PRESSED_PADDLE_LEFT)
                key = DIT;
            else if (radioState.paddleOrientation != NORMAL && buttonPressed == BTN_PRESSED_PADDLE_RIGHT)
                key = DIT;
            else if (radioState.paddleOrientation == NORMAL && buttonPressed == BTN_PRESSED_PADDLE_RIGHT)
                key = DAH;
            else if (radioState.paddleOrientation != NORMAL && buttonPressed == BTN_PRESSED_PADDLE_LEFT)
                key = DAH;
            else if (squeeze == ENABLED) {
                squeeze = DISABLED;
                if (radioState.keyerIndex == IAMBIC_MODE_B)
                    (key == DAH) ? (key = DIT) : (key = DAH);
                else
                    break;
                }
            else
                break;
        }
        buttonPressed = BTN_PRESSED_NONE;
    } while (1);
    buttonPressed = BTN_PRESSED_NONE;
}

