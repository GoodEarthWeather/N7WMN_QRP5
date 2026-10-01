/*
 * init.c
 *
 *  Created on: June 26, 2019
 *      Author: dmcneill
 *//////
//#include "main.h"
#include "init.h"
#include "lcdLib.h"
#include "driverlib.h"
#include "radio_state.h"

#define SHIFTER_CLOCK   GPIO_PORT_P2, GPIO_PIN4
#define SHIFTER_DATA   GPIO_PORT_P2, GPIO_PIN6




//This file contains the routines to initialize everything
// QEX Amplifier Tester

// initialize the clock system
void initClocks(void)
{

    //Initialize external 32.768kHz clock
    CS_setExternalClockSource(32768);
    CS_turnOnXT1LF(CS_XT1_DRIVE_3);

    //Set DCO frequency to 8MHz
    CS_initClockSignal(CS_FLLREF,CS_XT1CLK_SELECT,CS_CLOCK_DIVIDER_1);
    CS_initFLLSettle(8000,244);  // 244*32.768 is approximately 8000kHz = 8MHz
    //Set ACLK = External 32.768kHz clock with frequency divider of 1
    CS_initClockSignal(CS_ACLK,CS_XT1CLK_SELECT,CS_CLOCK_DIVIDER_1);
    //Set SMCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_SMCLK,CS_DCOCLKDIV_SELECT,CS_CLOCK_DIVIDER_1);
    //Set MCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_MCLK,CS_DCOCLKDIV_SELECT,CS_CLOCK_DIVIDER_1);

    //Clear all OSC fault flag
    CS_clearAllOscFlagsWithTimeout(1000);
}

// initialize GPIO
void initGPIO(void)
{
  /*
   * Set Pin 2.0, 2.1 to input Primary Module Function, LFXT.
   * This is for configuration of the external 32.768kHz crystal
   */
   GPIO_setAsPeripheralModuleFunctionInputPin(
       GPIO_PORT_P2,
       GPIO_PIN0 + GPIO_PIN1,
       GPIO_PRIMARY_MODULE_FUNCTION
   );
   // Configure Pins for I2C
   //Set P1.3 and P1.2 as Primary Module Function Input.
   //Select Port 1
   //Set Pin 2, 3 to input Primary Module Function, (UCB0SIMO/UCB0SDA, UCB0SOMI/UCB0SCL).
  GPIO_setAsPeripheralModuleFunctionInputPin(
      GPIO_PORT_P1,
      GPIO_PIN2 + GPIO_PIN3,
      GPIO_PRIMARY_MODULE_FUNCTION
      );

   // 1. Configure Hardware SPI Pins for eUSCI_B0
   // P2.4 = UCA1CLK (Clock Out)
   // P2.6 = UCA1SIMO (Data Out)
   // GPIO_PRIMARY_MODULE_FUNCTION activates the internal eUSCI hardware multiplexer
   GPIO_setAsPeripheralModuleFunctionOutputPin(SHIFTER_CLOCK,GPIO_PRIMARY_MODULE_FUNCTION);
   GPIO_setAsPeripheralModuleFunctionOutputPin(SHIFTER_DATA,GPIO_PRIMARY_MODULE_FUNCTION);
   GPIO_setAsOutputPin(REG_CLK);
   GPIO_setOutputLowOnPin(REG_CLK);

   //Initialize rotary encoder inputs and rotary encoder switches
   // P5.0(switch), P5.1(A), P5.2(B) - for LED encoder
   // P4.7(A), P6.2(B), P6.1(switch) for menu encoder
   // P4.5(A), P2.2(B), P4.6(xwitch) for tuner encoder
   GPIO_setAsInputPin(LED_ENCODER_A);
   GPIO_setAsInputPin(LED_ENCODER_B);
   GPIO_setAsInputPin(LED_ENCODER_SWITCH);
   GPIO_setAsInputPin(MENU_ENCODER_A);
   GPIO_setAsInputPin(MENU_ENCODER_B);
   GPIO_setAsInputPin(MENU_ENCODER_SWITCH);
   GPIO_setAsInputPin(TUNER_ENCODER_A);
   GPIO_setAsInputPin(TUNER_ENCODER_B);
   GPIO_setAsInputPin(TUNER_ENCODER_SWITCH);

   GPIO_selectInterruptEdge(LED_ENCODER_A, GPIO_HIGH_TO_LOW_TRANSITION);  // interrupt on falling edge of menu encoder pin A
   GPIO_selectInterruptEdge(LED_ENCODER_SWITCH, GPIO_HIGH_TO_LOW_TRANSITION);  // interrupt on falling edge of menu encoder switch
   GPIO_selectInterruptEdge(MENU_ENCODER_A, GPIO_HIGH_TO_LOW_TRANSITION);  // interrupt on falling edge of menu option encoder pin A
   GPIO_selectInterruptEdge(MENU_ENCODER_SWITCH, GPIO_HIGH_TO_LOW_TRANSITION);  // interrupt on falling edge of menu option encoder switch
   GPIO_selectInterruptEdge(TUNER_ENCODER_A, GPIO_HIGH_TO_LOW_TRANSITION);  // interrupt on falling edge of tuner encoder pin A
   GPIO_selectInterruptEdge(TUNER_ENCODER_SWITCH, GPIO_HIGH_TO_LOW_TRANSITION);  // interrupt on falling edge of tuner encoder switch

   // Configure interrupts for encoder
   GPIO_enableInterrupt(LED_ENCODER_A);
   GPIO_clearInterrupt(LED_ENCODER_A);
   GPIO_enableInterrupt(LED_ENCODER_SWITCH);
   GPIO_clearInterrupt(LED_ENCODER_SWITCH);

   GPIO_enableInterrupt(MENU_ENCODER_A);
   GPIO_clearInterrupt(MENU_ENCODER_A);
   GPIO_enableInterrupt(MENU_ENCODER_SWITCH);
   GPIO_clearInterrupt(MENU_ENCODER_SWITCH);

   GPIO_enableInterrupt(TUNER_ENCODER_A);
   GPIO_clearInterrupt(TUNER_ENCODER_A);
   GPIO_enableInterrupt(TUNER_ENCODER_SWITCH);
   GPIO_clearInterrupt(TUNER_ENCODER_SWITCH);

   // initalize LCD I/O
   GPIO_setAsOutputPin(LCD_D4);
   GPIO_setOutputLowOnPin(LCD_D4);
   GPIO_setAsOutputPin(LCD_D5);
   GPIO_setOutputLowOnPin(LCD_D5);
   GPIO_setAsOutputPin(LCD_D6);
   GPIO_setOutputLowOnPin(LCD_D6);
   GPIO_setAsOutputPin(LCD_D7);
   GPIO_setOutputLowOnPin(LCD_D7);
   GPIO_setAsOutputPin(LCD_RS);
   GPIO_setOutputLowOnPin(LCD_RS);
   GPIO_setAsOutputPin(LCD_CLK);
   GPIO_setOutputLowOnPin(LCD_CLK);


   GPIO_setAsOutputPin(POWER_DRV_ENABLE);
   GPIO_setOutputLowOnPin(POWER_DRV_ENABLE);
   GPIO_setAsOutputPin(FILTER_SELECT);
   GPIO_setOutputLowOnPin(FILTER_SELECT);
   GPIO_setAsOutputPin(SELECT_BINAURAL);
   GPIO_setOutputLowOnPin(SELECT_BINAURAL); // default to monaural
   GPIO_setAsInputPin(BTN_TXMODE);
   GPIO_setAsOutputPin(TX_LED);
   GPIO_setOutputLowOnPin(TX_LED);
   GPIO_setAsOutputPin(POWER_AMP_ENABLE);
   GPIO_setOutputLowOnPin(POWER_AMP_ENABLE);
   GPIO_setAsOutputPin(SPARE_P3P3);
   GPIO_setOutputLowOnPin(SPARE_P3P3);
   GPIO_setAsOutputPin(SPARE_P2P3);
   GPIO_setOutputLowOnPin(SPARE_P2P3);
   GPIO_setAsOutputPin(SPARE_P3P4);
   GPIO_setOutputLowOnPin(SPARE_P3P4);
   GPIO_setAsOutputPin(SPARE_P3P1);
   GPIO_setOutputLowOnPin(SPARE_P3P1);
   GPIO_setAsOutputPin(SPARE_P2P5);
   GPIO_setOutputLowOnPin(SPARE_P2P5);
   GPIO_setAsOutputPin(SPARE_P3P7);
   GPIO_setOutputLowOnPin(SPARE_P3P7);
   GPIO_setAsOutputPin(POWER_RF_ENABLE);
   GPIO_setOutputHighOnPin(POWER_RF_ENABLE); // set high to tri-state buffers

   GPIO_setAsInputPin(PADDLE_LEFT);
   GPIO_setAsInputPin(PADDLE_RIGHT);
   GPIO_setAsInputPin(STRAIGHT_KEY);

   GPIO_setAsOutputPin(TR_SWITCH);
   GPIO_setOutputHighOnPin(TR_SWITCH); // set high for receive mode
   GPIO_setAsOutputPin(CWTX_OUT);
   GPIO_setOutputLowOnPin(CWTX_OUT);
   GPIO_setAsOutputPin(MUTE_OUT);
   GPIO_setOutputLowOnPin(MUTE_OUT);

   // configure cw key interrupts
   GPIO_selectInterruptEdge(PADDLE_LEFT, GPIO_HIGH_TO_LOW_TRANSITION);  // interrupt on falling edge of dit key
   GPIO_selectInterruptEdge(PADDLE_RIGHT, GPIO_HIGH_TO_LOW_TRANSITION);  // interrupt on falling edge of dit key
   GPIO_enableInterrupt(PADDLE_LEFT);
   GPIO_clearInterrupt(PADDLE_LEFT);
   GPIO_enableInterrupt(PADDLE_RIGHT);
   GPIO_clearInterrupt(PADDLE_RIGHT);


   // Initialize side tone output
   GPIO_setAsPeripheralModuleFunctionOutputPin(SIDETONE_OUTPUT,GPIO_SECONDARY_MODULE_FUNCTION);
   GPIO_setOutputLowOnPin(SIDETONE_OUTPUT);  // set side tone output low

   // Configure P1.0 (A0) as analog ADC input for VBAT sensing
   GPIO_setAsPeripheralModuleFunctionInputPin(VDD_SENSE, GPIO_TERNARY_MODULE_FUNCTION);

   /*
    * Disable the GPIO power-on default high-impedance mode to activate
    * previously configured port settings
    */
   PMM_unlockLPM5();
   // Enable Global Interrupts (GIE bit in Status Register)
   __enable_interrupt();

}

void init_spi_shift_register(void)
{


    // 3. Initialize eUSCI_A1 SPI Master Configuration Structure
    EUSCI_A_SPI_initMasterParam param = {0};
    param.selectClockSource = EUSCI_A_SPI_CLOCKSOURCE_SMCLK;             // Use Sub-Main Clock
    param.clockSourceFrequency = CS_getSMCLK();                          // Grab current SMCLK frequency automatically
    param.desiredSpiClock = 1000000;                                     // Drive transmission at 1 MHz
    param.msbFirst = EUSCI_A_SPI_MSB_FIRST;                              // 74HCT595 shifts in MSB first
    param.clockPolarity = EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW;      // CPOL = 0 (Clock low when idle)
    param.clockPhase = EUSCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT; // CPHA = 0 (Data changes on falling, latched on rising edge)
    param.spiMode = EUSCI_A_SPI_3PIN;                                    // 3-Wire Mode (Clock, SIMO, ignoring SOMI line)

    // 4. Initialize and Enable the eUSCI_A1 Peripheral Module
    EUSCI_A_SPI_initMaster(EUSCI_A1_BASE, &param);
    EUSCI_A_SPI_enable(EUSCI_A1_BASE);

    // Clear residual eUSCI_A1 RX flag interrupts before starting
    EUSCI_A_SPI_clearInterrupt(EUSCI_A1_BASE, EUSCI_A_SPI_RECEIVE_INTERRUPT);

}

// initialize timer A0 for up mode - for side tone
void initSideToneTimer(void)
{

    // timer is clocked by 32768 clock
    // (1/600Hz)/(1/32768Hz) is about 55 counts, so set compare threshold to 55
    //Start timer in up mode sourced by ACLK
    Timer_A_initUpModeParam initUpParam = {0};
    initUpParam.clockSource = TIMER_A_CLOCKSOURCE_ACLK;
    initUpParam.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    initUpParam.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    initUpParam.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE;
    initUpParam.timerClear = TIMER_A_DO_CLEAR;
    initUpParam.startTimer = false;
    initUpParam.timerPeriod = (uint16_t)(26);
    Timer_A_initUpMode(TIMER_A0_BASE, &initUpParam);
    Timer_A_setOutputMode(TIMER_A0_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_1,TIMER_A_OUTPUTMODE_TOGGLE);
    Timer_A_setOutputForOutputModeOutBitValue(TIMER_A0_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_1,TIMER_A_OUTPUTMODE_OUTBITVALUE_LOW);

    //Initiaze compare mode
    Timer_A_clearCaptureCompareInterrupt(TIMER_A0_BASE,
        TIMER_A_CAPTURECOMPARE_REGISTER_1
        );
}
// initialize timer A1 for continuous mode - for QSK timing
void initQSKTimer(uint16_t delay)
{
    uint16_t compareValue;
    // convert delay in milliseconds to a compare value
    compareValue = (uint16_t)((float)delay*32.768);

    // use timer A1
    //Start timer in continuous mode sourced by AMCLK
    Timer_A_initContinuousModeParam initContParam = {0};
    initContParam.clockSource = TIMER_A_CLOCKSOURCE_ACLK;
    initContParam.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    initContParam.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    initContParam.timerClear = TIMER_A_DO_CLEAR;
    initContParam.startTimer = false;
    Timer_A_initContinuousMode(TIMER_A1_BASE, &initContParam);

    //Initiaze compare mode
    Timer_A_clearCaptureCompareInterrupt(TIMER_A1_BASE,
        TIMER_A_CAPTURECOMPARE_REGISTER_0
        );

    Timer_A_initCompareModeParam initCompParam = {0};
    initCompParam.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_0;
    initCompParam.compareInterruptEnable = TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE;
    initCompParam.compareOutputMode = TIMER_A_OUTPUTMODE_OUTBITVALUE;
    initCompParam.compareValue = compareValue;
    Timer_A_initCompareMode(TIMER_A1_BASE, &initCompParam);
}


#pragma vector=TIMER1_A0_VECTOR
__interrupt
void TIMER1_A0_ISR (void)
{
    // QSK timeout reached, so unmute audio and stop timer
    Timer_A_stop(TIMER_A1_BASE);
    Timer_A_clearCaptureCompareInterrupt(TIMER_A1_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);
    Timer_A_disableCaptureCompareInterrupt(TIMER_A1_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);
    unmuteAudio();
}


// initialize timer A2 for up mode
void initKeyTimer(uint8_t wpm)
{
    uint16_t count;

    count = 39322/wpm;
    //Start timer in up mode sourced by ACLK
    Timer_A_initUpModeParam initUpParam = {0};
    initUpParam.clockSource = TIMER_A_CLOCKSOURCE_ACLK;
    initUpParam.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    initUpParam.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    initUpParam.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE;
    initUpParam.timerClear = TIMER_A_DO_CLEAR;
    initUpParam.startTimer = false;
    initUpParam.timerPeriod = count;
    Timer_A_initUpMode(TIMER_A2_BASE, &initUpParam);

    //Initiaze compare mode
    Timer_A_clearCaptureCompareInterrupt(TIMER_A2_BASE,
        TIMER_A_CAPTURECOMPARE_REGISTER_0
        );
    Timer_A_startCounter(TIMER_A2_BASE,TIMER_A_UP_MODE);  // start timer
}

/*******************************
// initialize timer A3 for continuous mode - for CW message recording timing
void initCWMsgRecordTimer(void)
{
    // use timer A3
    //configure timer in continuous mode sourced by ACLK (32.768 kHz)
    Timer_A_initContinuousModeParam initContParam = {0};
    initContParam.clockSource = TIMER_A_CLOCKSOURCE_ACLK;
    initContParam.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    initContParam.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    initContParam.timerClear = TIMER_A_DO_CLEAR;
    initContParam.startTimer = false;
    Timer_A_initContinuousMode(TIMER_A3_BASE, &initContParam);
}

// initialize timer A3 for up mode - for CW message playback timing
void initCWMsgPlayTimer(void)
{
    // use timer A3
    //configure timer in up mode sourced by ACLK (32.768 kHz)
    Timer_A_initUpModeParam initUpParam = {0};
    initUpParam.clockSource = TIMER_A_CLOCKSOURCE_ACLK;
    initUpParam.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    initUpParam.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    initUpParam.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE;
    initUpParam.timerClear = TIMER_A_DO_CLEAR;
    initUpParam.startTimer = false;
    //initUpParam.timerPeriod = count;
    Timer_A_initUpMode(TIMER_A3_BASE, &initUpParam);

    //Initialize compare mode
    Timer_A_clearCaptureCompareInterrupt(TIMER_A3_BASE,
        TIMER_A_CAPTURECOMPARE_REGISTER_0
        );

}
**************************/
