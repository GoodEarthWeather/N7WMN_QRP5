/*
 * radio_state.c
 */

#include "radio_state.h"
#include "lcdLib.h"
#include "menu_data.h"
#include "init.h"
#include "si5351.h"
#include "key.h"


static void updateRelayShifter(uint8_t);


#define BAND_FIELD 0x0D
#define MODE_FIELD 0x4D
#define BAND_FIELD_WIDTH 3
#define MODE_FIELD_WIDTH 3


static const LcdField_t fieldBand   = { BAND_FIELD, BAND_FIELD_WIDTH };
static const LcdField_t fieldMode = { MODE_FIELD, MODE_FIELD_WIDTH };

// define frequency bands
#define BAND_40M_LOWER 7000000
#define BAND_40M_UPPER 7300000
#define BAND_30M_LOWER 10100000
#define BAND_30M_UPPER 10150000
#define BAND_20M_LOWER 14000000
#define BAND_20M_UPPER 14350000
#define BAND_17M_LOWER 18068000
#define BAND_17M_UPPER 18168000
#define BAND_15M_LOWER 21000000
#define BAND_15M_UPPER 21450000
static uint32_t maxBand[] = {BAND_40M_UPPER, BAND_30M_UPPER, BAND_20M_UPPER, BAND_17M_UPPER, BAND_15M_UPPER};
static uint32_t minBand[] = {BAND_40M_LOWER, BAND_30M_LOWER, BAND_20M_LOWER, BAND_17M_LOWER, BAND_15M_LOWER};

// define rates - these numbers must match the order in the MenuItem_t definition
#define RATE_10 10
#define RATE_100 100
#define RATE_1K 1000
#define RATE_10K 10000

#define SPOT_OFF 0
#define SPOT_ON 1

#define MUTE_ON 0x1
#define MUTE_OFF 0x0
#define CW_FILTER 1
#define MONAURAL 0

#define RELAY_40M 0b01101001
#define RELAY_30M 0b10100110
#define RELAY_20M 0b10100110
#define RELAY_17M 0b10011010
#define RELAY_15M 0b10011010
// the indexes of relayCode[] correspond to the bandIndex
static uint8_t relayCode[] = {RELAY_40M, RELAY_30M, RELAY_20M, RELAY_17M, RELAY_15M};


#pragma PERSISTENT(radioState)
RadioState_t radioState = { 0 };
#pragma PERSISTENT(radioStateInitialized)
static uint8_t radioStateInitialized = 0xFF; /* 0xFF => "never initialized" */

/*
 * This function will only execute if the radioState data structure has never
 * been initialized.
 */
void initializeRadioState(void)
{
    if (radioStateInitialized != 0x01)
    {
        radioState.txMode = 0; // receive mode
        radioState.audioState = MUTE_OFF;
        radioState.frequency[0] = BAND_40M_LOWER;
        radioState.frequency[1] = BAND_30M_LOWER;
        radioState.frequency[2] = BAND_20M_LOWER;
        radioState.frequency[3] = BAND_17M_LOWER;
        radioState.frequency[4] = BAND_15M_LOWER;
        radioState.ritOffset = 0;
        radioState.xitOffset = 0;
        radioState.maxBandFreq = BAND_40M_UPPER;
        radioState.minBandFreq = BAND_40M_LOWER;
        radioState.ledIndex = 0;
        radioState.selectedSideband = UPPER_SIDEBAND;
        radioState.sidetoneFreq = 600;
        radioState.txKeyState = TX_KEY_UP;

        radioStateInitialized = 0x01;
    }

}
/*
 * This function is called upon power up; it will go through the menu data
 * structure and configure the HW to reflect each menu item.  Each callback
 * function will also set the corresponding variable in radioState.
 */
void initializeHW(void)
{
    // note that in the routine Menu_Init(), each of the callback functions associated
    // with each menu item was called and the hw was configured for each of those functions.
    // the pins below are not part of the menu system and are set up here
    GPIO_setOutputHighOnPin(POWER_RF_ENABLE);  // set high to disable rf clock to tri-state buffers
    GPIO_setOutputLowOnPin(POWER_DRV_ENABLE);  // set low to disable switching regulator for rf gate drivers
    GPIO_setOutputLowOnPin(POWER_AMP_ENABLE);  // set low to disable switching regulator for power output stage
    GPIO_setOutputHighOnPin(TR_SWITCH);  // set high to configure for receive mode
    GPIO_setOutputLowOnPin(TX_LED);  // set low to disable transmit led
    setSI5351Freq(radioState.frequency[radioState.bandIndex]);

}
/* ------------------------------------------------------------------ */
/* Hardware / state action callbacks -- ONE per menu item, no          */
/* exceptions. Even a "just set a variable" item gets a function here  */
/* so that Menu_OptionMove()/Menu_Init() never need to know what kind  */
/* of thing each item controls -- that knowledge lives ONLY in these   */
/* functions and nowhere else. Signature must match MenuActionFn in    */
/* menu_data.h.                                                        */
/* ------------------------------------------------------------------ */

void handleHW_wpm(const MenuItem_t *item, int16_t value)
{
    radioState.wpm = (uint8_t)value;
    initKeyTimer((uint8_t)value);  // update timer with new speed
}

void handleHW_QSK(const MenuItem_t *item, int16_t value)
{
    radioState.qsk = value;
    initQSKTimer(value);  // update timer with new speed
}

void handleHW_band(const MenuItem_t *item, int16_t value)
{

    const char *suffix = NULL;
    const char * bandName[] = {"40M", "30M", "20M", "17M", "15M"};

    if (radioState.audioState == MUTE_OFF)
        muteAudio();
    radioState.bandIndex = (uint8_t)value;
    radioState.bandRelayCode = relayCode[(uint8_t)value];
    radioState.maxBandFreq = maxBand[(uint8_t)value];
    radioState.minBandFreq = minBand[(uint8_t)value];
    LCD_WriteField(&fieldBand,bandName[(uint8_t)value],suffix);
    // now update shift register to select new band
    updateRelayShifter(radioState.bandRelayCode); // shift in relay code to change relay
    // now, delay 12ms for relay settling
    delay_ms(12);
    updateRelayShifter(0); // zero out relay code; magnetic latch holding relay

    // now, set the si5351 to the correct output frequency
    setSI5351Freq(radioState.frequency[(uint8_t)value]);
    // now update the LCD freq. field
    updateLCD_freq();
    delay_ms(3);
    if (radioState.audioState == MUTE_OFF)
        unmuteAudio();
}

// Routine to handle mode
void handleHW_mode(const MenuItem_t *item, int16_t value)
{
    const char * modeName[] = {" CW", "CWR", "USB", "LSB"};
    const uint8_t sideband[] = {UPPER_SIDEBAND, LOWER_SIDEBAND, UPPER_SIDEBAND, LOWER_SIDEBAND};
    const char *suffix = NULL;

    if (radioState.audioState == MUTE_OFF)
        muteAudio();
    radioState.modeIndex = (uint8_t)value;
    radioState.selectedSideband = sideband[value];
    LCD_WriteField(&fieldMode,modeName[(uint8_t)value],suffix);
    // write code to implement HW change of mode
    setSI5351Freq(radioState.frequency[radioState.bandIndex]);
    delay_ms(10);
    if (radioState.audioState == MUTE_OFF)
        unmuteAudio();
}

// Routine to handle audio filter
void handleHW_filter(const MenuItem_t *item, int16_t value)
{
    radioState.filterIndex = (uint8_t)value;
    // write code to implement HW change of filter
    if (value == CW_FILTER)
        GPIO_setOutputHighOnPin(FILTER_SELECT);  // set low for narrow filter
    else
        GPIO_setOutputLowOnPin(FILTER_SELECT); // set low for wide filter

}

// Routine to handle keyer option (iambic-a, iambic-b, ultimatic)
void handleHW_keyer(const MenuItem_t *item, int16_t value)
{
    radioState.keyerIndex = (uint8_t)value;
    // write code to implement SW change of keyer type
}

// Routine to handle paddle orientation (normal, reversed)
void handleHW_paddleOrientation(const MenuItem_t *item, int16_t value)
{
    radioState.paddleOrientation = (uint8_t)value;
    // write code to implement SW change of keyer type
}

// Routine to handle spot - not stored in radioState
void handleHW_spot(const MenuItem_t *item, int16_t value)
{
    if ((uint8_t)value == SPOT_OFF) {
        // handle turning spot off
        Timer_A_stop(TIMER_A0_BASE);  // stop side tone
        Timer_A_setOutputForOutputModeOutBitValue(TIMER_A0_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_1,TIMER_A_OUTPUTMODE_OUTBITVALUE_LOW);
    } else {
        // turning spot on
        Timer_A_startCounter(TIMER_A0_BASE,TIMER_A_UP_MODE);  // start side tone
    }
}

// Routine to mute audio - not stored in radioState
// This only mutes from the menu; internal muting is handled elsewhere
void handleHW_mute(const MenuItem_t *item, int16_t value)
{
    if ((uint8_t)value == MUTE_OFF) {
        GPIO_setOutputLowOnPin(MUTE_OUT); // set low to unmute
        radioState.audioState = MUTE_OFF;
    } else {
        GPIO_setOutputHighOnPin(MUTE_OUT); // set high to mute
        radioState.audioState = MUTE_ON;
    }
}

// Routine to select audio mode: binaural or monaural
void handleHW_audioMode(const MenuItem_t *item, int16_t value)
{
    if (radioState.audioState == MUTE_OFF)
        muteAudio();
    radioState.audioMode = (uint8_t)value;
    if ((uint8_t)value == MONAURAL) {
        GPIO_setOutputLowOnPin(SELECT_BINAURAL); // set low to select monaural
    } else {
        GPIO_setOutputHighOnPin(SELECT_BINAURAL); // set high to select binaural
    }
    delay_ms(10);
    if (radioState.audioState == MUTE_OFF)
        unmuteAudio();
}

/*
 * Routine to update rate
 */
void handleHW_rate(const MenuItem_t *item, int16_t value)
{
    const uint32_t rateValues[] = {10,100,1000,10000};
    // value is the index into the *item list
    radioState.freqMultiplier = rateValues[(uint8_t)value];
    // not using visible cursor for now, so don't need to call this:
    //moveFreqCursor((uint8_t)value);
}

// This routine will update  the latching relays for the filters
// Sends '4' bytes out to a chain of cascaded 74HCT595s and latches once.
// data[3] = byte for filter latching relays - first chip
// data[2] = byte for the second chip in the chain (closest to MCU, SER pin)
// data[1] = byte for the third chip
// data[0] = byte for the fourth chip
// (byte order is reversed internally since the chain shifts "backwards")
static void updateRelayShifter(uint8_t relayCode)
{
    uint8_t i;
    uint8_t data[4];
    uint32_t selectedLED = 0;
    extern RadioState_t radioState;

    selectedLED = (1UL << radioState.ledIndex);  // convert number to bit
    selectedLED = ~selectedLED;  // invert all bits to match HW implementation of turning on LED
    // construct bytes to send
    data[3] = relayCode;
    data[2] = (uint8_t)(selectedLED  & 0x000000FF);
    data[1] = (uint8_t)((selectedLED >> 8) & 0x000000FF);
    data[0] = (uint8_t)((selectedLED >> 16) & 0x000000FF);
    // Send last chip's byte first, first chip's byte last
    for (i = 0; i < 4; i++)
    {
        while (!EUSCI_A_SPI_getInterruptStatus(EUSCI_A1_BASE, EUSCI_A_SPI_TRANSMIT_INTERRUPT));
        EUSCI_A_SPI_transmitData(EUSCI_A1_BASE, data[i]);
    }
    // Wait for the last byte to fully clock out before latching
    while (EUSCI_A_SPI_isBusy(EUSCI_A1_BASE));

    // Latch all 24 bits to the outputs simultaneously
    GPIO_setOutputHighOnPin(REG_CLK);
    __delay_cycles(10);
    GPIO_setOutputLowOnPin(REG_CLK);
}

void muteAudio(void)
{
    GPIO_setOutputHighOnPin(MUTE_OUT); // set high to mute
}

void unmuteAudio(void)
{
    GPIO_setOutputLowOnPin(MUTE_OUT); // set low to unmute
}

