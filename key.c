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

/*************************
// This routine will play the cw message in the cwMsg[] vector
void playCwMsg(uint8_t mem)
{
    extern uint16_t *cwMemPtr;
    extern cwMem_t cwMsg;
    //extern uint16_t cwMsg[];
    extern uint8_t txMode;
    extern uint8_t volatile buttonPressed;
    extern uint8_t selectedMem;  // selected cw message memory
    uint16_t count;
    uint8_t done;
    uint8_t on = 0;

    // copy message from nvs memory to cwMsg.mem
    cwMemRead(mem);

    // wait until a dit or dah or memory button is pressed
    // a dit or dah will initiate sending the memory message
    // a memory button push will go to the next memory, or will exit back to normal display
    while ((buttonPressed != BTN_PRESSED_DIT) && (buttonPressed != BTN_PRESSED_DAH) && (buttonPressed != BTN_PRESSED_MEMORY)) {;}
    if (buttonPressed != BTN_PRESSED_MEMORY)
    {
        buttonPressed = BTN_PRESSED_NONE;
        cwMemPtr = cwMsg.mem;  // set pointer to beginning of memory vector
        cwMemPtr++;  // skip the 0 in the first location

        // configure message timer (A3)
        initCWMsgPlayTimer();
        on = 0;
        while ( (count = *cwMemPtr++) && (buttonPressed == BTN_PRESSED_NONE) )
        {
            on = !on;  // switch to on or off
            Timer_A_clear(TIMER_A3_BASE);  // clear timer
            Timer_A_setCompareValue (TIMER_A3_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0, count);
            if (on)
            {
                Timer_A_startCounter(TIMER_A0_BASE,TIMER_A_UP_MODE);  // start side tone
                if (txMode == ENABLED)
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
                if (txMode == ENABLED)
                    keyUp();
            }
        }
        buttonPressed = BTN_PRESSED_NONE;  // if playback was interrupted by keypress, ignore the key that was pressed
        updateDisplay(MENU_DISPLAY);
    } else {
        if (selectedMem == MEM3)
        {
            selectedMem = MEM1;
            updateDisplay(MENU_DISPLAY);
            buttonPressed = BTN_PRESSED_NONE;
        } else {
        selectedMem++;
        updateDisplay(PLAY_MEM_DISPLAY);
        }
    }
}

void recordCwMsg(uint8_t mem)
{
    extern uint8_t volatile buttonPressed;
    extern uint8_t paddleOrientation;  // paddles configured for dah-dit or dit-dah
    extern uint8_t cwMsgState;  // either disabled, recording or playing
    extern cwMem_t cwMsg;
    extern uint16_t *cwMemPtr;

    cwMemPtr = cwMsg.mem;  // reset pointer to start of vector
    while ( (buttonPressed != BTN_PRESSED_ENCODER) )
    {
        switch (buttonPressed)
        {
        case BTN_PRESSED_DIT :
            buttonPressed = BTN_PRESSED_NONE;
            (paddleOrientation == PADDLE_DAH_DIT) ? ditdah(DIT) : ditdah(DAH);
            break;
        case BTN_PRESSED_DAH :
            buttonPressed = BTN_PRESSED_NONE;
            (paddleOrientation == PADDLE_DAH_DIT) ? ditdah(DAH) : ditdah(DIT);
            break;
        default :
            break;
        }
    }
    *cwMemPtr = 0; // put a zero at the end of the message
    cwMemWrite(mem);  // save message into nvs
    buttonPressed = BTN_PRESSED_NONE;
}
**********************/
/**********************
After completion of dit/dah:
if iambic-A:
    if dah was sent:
        if dah is still pressed:
            if dit is pressed:
                send dit
                back to top
            else
                send another dah
                back to top
        else if dit is pressed
            send dit
            back to top
        else
            exit function
    if dit was sent:
        if dit is still pressed:
            if dah is pressed:
                send dah
                back to top
            else
                send another dit
                back to top
        else if dah is pressed
            send dah
            back to top
        else
            exit function
else if iambic-B
    if dah was sent:
        if dah is still pressed:
            if BTN_PRESSED_DIT is true OR dit key is low : (dit key was pressed at some point during dah)
                set BTN_PRESSED_NONE
                send dit
                back to top
            else
                send another dah
                back to top
        else if BTN_PRESSED_DIT is true OR dit key is low: (dit key was pressed at some point during dah)
            set BTN_PRESSED_NONE
            send dit
            back to top
        else
            exit function
    if dit was sent:
        if dit is still pressed:
            if BTN_PRESSED_DAH is true OR dah key is low: (dah key was pressed at some point during dit)
                set BTN_PRESSED_NONE
                send dah
                back to top
            else
                send another dit
                back to top
        else if BTN_PRESSED_DAH is true OR dah key is low: (dah key was pressed at some point during dit)
            set BTN_PRESSED_NONE
            send dah
            back to top
        else
            exit function

 */

// This routine is to handle the dit and dah key
// 'key' is either DIT or DAH (i.e. 1 or 3)
void ditdah(uint8_t key)
{
    uint8_t done;
    uint8_t count;
    uint8_t ditKeyState;
    uint8_t dahKeyState;
    uint8_t squeeze = DISABLED;
    //extern uint8_t cwMsgState;  // indicates if recording cw message
    //extern uint16_t *cwMemPtr;
    extern uint8_t volatile buttonPressed;

    do {

        // start the sidetone and, if needed, cw message timer
        Timer_A_startCounter(TIMER_A0_BASE,TIMER_A_UP_MODE);  // start side tone
        /*******
        if (cwMsgState == RECORD)
        {
            Timer_A_stop(TIMER_A3_BASE);  // stop cw msg timer
            if (Timer_A_getInterruptStatus(TIMER_A3_BASE) == TIMER_A_INTERRUPT_PENDING)
            {
                // timer overflow - limit delay to max (2 seconds)
                *cwMemPtr++ = 0xFFFF;
                Timer_A_clearTimerInterrupt(TIMER_A3_BASE);
            } else {
                *cwMemPtr++ = Timer_A_getCounterValue(TIMER_A3_BASE);
            }
            Timer_A_clear(TIMER_A3_BASE);  // clear timer
            Timer_A_startCounter(TIMER_A3_BASE,TIMER_A_CONTINUOUS_MODE);  // start measuring keyDown time
        ************/

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
        /****************
        if (cwMsgState == RECORD)
        {
            Timer_A_stop(TIMER_A3_BASE);  // stop cw msg timer
            *cwMemPtr++ = Timer_A_getCounterValue(TIMER_A3_BASE);
            Timer_A_clear(TIMER_A3_BASE);  // clear timer
            Timer_A_startCounter(TIMER_A3_BASE,TIMER_A_CONTINUOUS_MODE);  // start measuring keyUp time
        }
        *****************/
        // Wait one dit time
        Timer_A_clear(TIMER_A2_BASE);  // clear dit/dah timer
        Timer_A_clearCaptureCompareInterrupt(TIMER_A2_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
        do {
            done = Timer_A_getCaptureCompareInterruptStatus(TIMER_A2_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0,TIMER_A_CAPTURECOMPARE_INTERRUPT_FLAG);
        } while (done != TIMER_A_CAPTURECOMPARE_INTERRUPT_FLAG);
        Timer_A_clearCaptureCompareInterrupt(TIMER_A2_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);

        // completion of key sent; now determine what to do next
        if (radioState.paddleOrientation == NORMAL){
            ditKeyState = GPIO_getInputPinValue(DIT_KEY);
            dahKeyState = GPIO_getInputPinValue(DAH_KEY);
        }
        else {
            ditKeyState = GPIO_getInputPinValue(DAH_KEY);
            dahKeyState = GPIO_getInputPinValue(DIT_KEY);
        }

        if (ditKeyState == GPIO_INPUT_PIN_LOW && dahKeyState == GPIO_INPUT_PIN_LOW){
            squeeze = ENABLED;
            if (radioState.keyerIndex != IAMBIC_MODE_ULTIMATIC){
                (key == DAH) ? (key = DIT) : (key = DAH);  // alternate dit/dah
            } else if (buttonPressed != BTN_PRESSED_NONE){
                (buttonPressed == BTN_PRESSED_DIT) ? (key = DIT) : (key = DAH);
            }
        } else if (ditKeyState == GPIO_INPUT_PIN_LOW && dahKeyState == GPIO_INPUT_PIN_HIGH) {
            squeeze = DISABLED;
            key = DIT;
        } else if (ditKeyState == GPIO_INPUT_PIN_HIGH && dahKeyState == GPIO_INPUT_PIN_LOW) {
            squeeze = DISABLED;
            key = DAH;
        } else  {  // both keys are high
            if (buttonPressed == BTN_PRESSED_DIT) {
                key = DIT;
            } else if (buttonPressed == BTN_PRESSED_DAH) {
                key = DAH;
            } else if (squeeze == ENABLED) {
                squeeze = DISABLED;
                if (radioState.keyerIndex == IAMBIC_MODE_B) {
                    (key == DAH) ? (key = DIT) : (key = DAH);
                }
           } else {
                break;
            }
        }
        buttonPressed = BTN_PRESSED_NONE;
    } while (1);
    buttonPressed = BTN_PRESSED_NONE;
}
/*
void ditdah(uint8_t key)
{
    uint8_t done;
    uint8_t count;
    uint8_t ditKeyState;
    uint8_t dahKeyState;
    uint8_t squeeze = DISABLED;
    extern uint8_t txMode;
    extern uint8_t paddleOrientation;  // paddles configured for dah-dit or dit-dah
    extern uint8_t cwMsgState;  // indicates if recording cw message
    extern uint16_t *cwMemPtr;
    extern uint8_t volatile buttonPressed;

    do {

        // start the sidetone and, if needed, cw message timer
        Timer_A_startCounter(TIMER_A0_BASE,TIMER_A_UP_MODE);  // start side tone
        if (cwMsgState == RECORD)
        {
            Timer_A_stop(TIMER_A3_BASE);  // stop cw msg timer
            if (Timer_A_getInterruptStatus(TIMER_A3_BASE) == TIMER_A_INTERRUPT_PENDING)
            {
                // timer overflow - limit delay to max (2 seconds)
                *cwMemPtr++ = 0xFFFF;
                Timer_A_clearTimerInterrupt(TIMER_A3_BASE);
            } else {
                *cwMemPtr++ = Timer_A_getCounterValue(TIMER_A3_BASE);
            }
            Timer_A_clear(TIMER_A3_BASE);  // clear timer
            Timer_A_startCounter(TIMER_A3_BASE,TIMER_A_CONTINUOUS_MODE);  // start measuring keyDown time
        }

        // if transmitter enabled, turn on
        if (txMode == ENABLED)
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
        if (txMode == ENABLED)
            keyUp();
        Timer_A_stop(TIMER_A0_BASE);  // stop side tone
        if (cwMsgState == RECORD)
        {
            Timer_A_stop(TIMER_A3_BASE);  // stop cw msg timer
            *cwMemPtr++ = Timer_A_getCounterValue(TIMER_A3_BASE);
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
        if (paddleOrientation == PADDLE_DAH_DIT){
            ditKeyState = GPIO_getInputPinValue(DIT_KEY);
            dahKeyState = GPIO_getInputPinValue(DAH_KEY);
        }
        else {
            ditKeyState = GPIO_getInputPinValue(DAH_KEY);
            dahKeyState = GPIO_getInputPinValue(DIT_KEY);
        }

        if (ditKeyState == GPIO_INPUT_PIN_LOW && dahKeyState == GPIO_INPUT_PIN_LOW){
            squeeze = ENABLED;
            if (iambicMode != IAMBIC_MODE_ULTIMATIC){
                (key == DAH) ? (key = DIT) : (key = DAH);  // alternate dit/dah
            } else if (buttonPressed != BTN_PRESSED_NONE){
                (buttonPressed == BTN_PRESSED_DIT) ? (key = DIT) : (key = DAH);
            }
        } else if (ditKeyState == GPIO_INPUT_PIN_LOW && dahKeyState == GPIO_INPUT_PIN_HIGH) {
            squeeze = DISABLED;
            key = DIT;
        } else if (ditKeyState == GPIO_INPUT_PIN_HIGH && dahKeyState == GPIO_INPUT_PIN_LOW) {
            squeeze = DISABLED;
            key = DAH;
        } else  {  // both keys are high
            if (squeeze == ENABLED) {
                squeeze = DISABLED;
                if (iambicMode == IAMBIC_MODE_B) {
                    (key == DAH) ? (key = DIT) : (key = DAH);
                }
            } else if (buttonPressed == BTN_PRESSED_DIT) {
                key = DIT;
            } else if (buttonPressed == BTN_PRESSED_DAH) {
                key = DAH;
            } else {
                break;
            }
        }
        buttonPressed = BTN_PRESSED_NONE;
    } while (1);
    buttonPressed = BTN_PRESSED_NONE;
}
*/
