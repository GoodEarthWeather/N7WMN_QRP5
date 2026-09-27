/*
 * radio_state.c
 */

#include "radio_state.h"
#include "lcdLib.h"
#include "menu_data.h"
#include "init.h"


//static void selectFilter(uint8_t);
//static void selectSideband(uint8_t);
//static void selectAudioState(uint8_t);
static void updateRelayShifter(uint8_t);

#define FREQ_FIELD 0x00
#define BAND_FIELD 0x0D
#define MODE_FIELD 0x4D
#define FREQ_FIELD_WIDTH 12
#define BAND_FIELD_WIDTH 3
#define MODE_FIELD_WIDTH 3

static const LcdField_t fieldFreq   = { FREQ_FIELD, FREQ_FIELD_WIDTH };
static const LcdField_t fieldBand   = { BAND_FIELD, BAND_FIELD_WIDTH };
static const LcdField_t fieldMode = { MODE_FIELD, MODE_FIELD_WIDTH };


// define rates - these numbers must match the order in the MenuItem_t definition
#define RATE_10 10
#define RATE_100 100
#define RATE_1K 1000
#define RATE_10K 10000
// define bands - these numbers must match the order in the MenuItem_t definition
// #define BAND_40M 0   //=> this is defined in radio_state.h because it is used by lcdLib.c
#define BAND_30M 1
#define BAND_20M 2
#define BAND_17M 3
#define BAND_15M 4

#define SPOT_OFF 0
#define SPOT_ON 1

#define MUTE 0x1
#define UNMUTE 0x0

#define RELAY_40M 0b01101001
#define RELAY_30M 0b10100110
#define RELAY_20M 0b10100110
#define RELAY_17M 0b10011010
#define RELAY_15M 0b10011010

// the indexes of relayCode[] correspond to the bandIndex
static uint8_t relayCode[] = {RELAY_40M, RELAY_30M, RELAY_20M, RELAY_17M, RELAY_15M};



RadioState_t radioState = { 0 };

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

    // Since the menu item for CW speed is a range value, it has
    // already been incremented or decremented by the option_move
    // function.
    radioState.wpm = (uint8_t)value;
    //initKeyTimer((uint8_t)value);  // update timer with new speed
}

void handleHW_band(const MenuItem_t *item, int16_t value)
{

    const char *suffix = NULL;
    const char * bandName[] = {"40M", "30M", "20M", "17M", "15M"};

    radioState.bandIndex = (uint8_t)value;
    radioState.bandRelayCode = relayCode[(uint8_t)value];
    LCD_WriteField(&fieldBand,bandName[(uint8_t)value],suffix);
    // now update shift register to select new band
    updateRelayShifter(radioState.bandRelayCode); // shift in relay code to change relay
    // now, delay 12ms for relay settling
    delay_ms(12);
    updateRelayShifter(0); // zero out relay code; magnetic latch holding relay

    //selectAudioState(MUTE);
    /*
    // reset menu function
    ritState = DISABLED;
    ritOffset = 0;
    receiveMode = RXMODE_CW;
    initADC(BATTERY_MEASUREMENT);
    *
    */
}

// Routine to handle mode
void handleHW_mode(const MenuItem_t *item, int16_t value)
{
    const char * modeName[] = {" CW", "USB", "LSB"};
    const char *suffix = NULL;
    radioState.modeIndex = (uint8_t)value;
    LCD_WriteField(&fieldMode,modeName[(uint8_t)value],suffix);
    // write code to implement HW change of mode
}

// Routine to handle audio filter
void handleHW_filter(const MenuItem_t *item, int16_t value)
{
    radioState.filterIndex = (uint8_t)value;
    // write code to implement HW change of filter
}

// Routine to handle keyer option (iambic-a, iambic-b, ultimatic)
void handleHW_keyer(const MenuItem_t *item, int16_t value)
{
    radioState.keyerIndex = (uint8_t)value;
    // write code to implement SW change of keyer type
}

// Routine to handle spot
void handleHW_spot(const MenuItem_t *item, int16_t value)
{
    if ((uint8_t)value == SPOT_OFF) {
        // handle turning spot off
        ;
    } else {
        // handle turning spot on
        ;
    }
    // write code to implement SW change of keyer type
}

/*
 * Routine to update rate
 */
void handleHW_rate(const MenuItem_t *item, int16_t value)
{
    const uint32_t rateValues[] = {10,100,1000,10000};
    // value is the index into the *item list
    radioState.freqMultiplier = rateValues[value];
    moveFreqCursor((uint8_t)value);
}

/*******************
// routine to select filter
static void selectFilter(uint8_t filter)
{
    radioState.selectedFilter = filter;
    if (filter == CW_FILTER)
        GPIO_setOutputLowOnPin(FILTER_SELECT);  // set low for CW filter
    else
        GPIO_setOutputHighOnPin(FILTER_SELECT); // set high for SSB filter
}

// routine to select sideband
static void selectSideband(uint8_t sideband)
{
    radioState.selectedSideband = sideband;
    if (sideband == UPPER_SIDEBAND)
        GPIO_setOutputHighOnPin(SIDEBAND_SELECT);  // need to check
    else
        GPIO_setOutputLowOnPin(SIDEBAND_SELECT); // need to check
}

// routine to set audio state - mute or unmute
static void selectAudioState(uint8_t state)
{
    radioState.audioState = state;
    if ( state == MUTE )
        GPIO_setOutputHighOnPin(TR_MUTE); // set high for mute pin
    else if (state == UNMUTE)
        GPIO_setOutputLowOnPin(TR_MUTE); // set low for unmute pin
}
***************/
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

