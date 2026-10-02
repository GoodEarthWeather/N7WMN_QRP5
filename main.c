


#include "driverlib.h"
//#include "main.h"
#include "isr.h"
#include "menu_data.h"
#include "radio_state.h"
#include "init.h"
#include "lcdLib.h"
#include "si5351.h"
#include "key.h"

static void handle_led_encoder(void);
static void handle_menu_encoder(void);
static void handle_menu_encoder_switch(void);
static void updateLEDShifter( uint8_t);
static void handle_tuner_encoder(void);


int main(void) {

    WDT_A_hold(WDT_A_BASE);
    initGPIO();
    initClocks();
    lcdInit();
    initSideToneTimer();
    initialize_si5351();
    si5351_disable_spread_spectrum();
    Menu_Init();
    updateLCD_status();
    init_spi_shift_register();
    initializeRadioState();
    initializeHW();
    updateLEDShifter(0);  // initialize led array

    while (1)
    {
        if (buttonPressed != BTN_PRESSED_NONE)
        {
            switch (buttonPressed)
            {
            case BTN_PRESSED_NONE :
                break;
            case BTN_PRESSED_LED_ENCODER_SWITCH:
                buttonPressed = BTN_PRESSED_NONE;
                break;
            case BTN_PRESSED_MENU_ENCODER_SWITCH :
                handle_menu_encoder_switch();
                buttonPressed = BTN_PRESSED_NONE;
                break;
            case BTN_PRESSED_TUNER_ENCODER_SWITCH :
                buttonPressed = BTN_PRESSED_NONE;
                break;
            case BTN_PRESSED_LED_ENCODER :
                handle_led_encoder();
                __disable_interrupt();
                if (buttonPressed == BTN_PRESSED_LED_ENCODER) {
                    buttonPressed = BTN_PRESSED_NONE;
                }
                __enable_interrupt();
                break;
            case BTN_PRESSED_MENU_ENCODER :
                handle_menu_encoder();
                __disable_interrupt();
                if (buttonPressed == BTN_PRESSED_MENU_ENCODER) {
                    buttonPressed = BTN_PRESSED_NONE;
                }
                __enable_interrupt();
                break;
            case BTN_PRESSED_TUNER_ENCODER :
                handle_tuner_encoder();
                __disable_interrupt();
                if (buttonPressed == BTN_PRESSED_TUNER_ENCODER) {
                    buttonPressed = BTN_PRESSED_NONE;
                }
                __enable_interrupt();
                break;
            case BTN_PRESSED_PADDLE_LEFT :
                buttonPressed = BTN_PRESSED_NONE;
                (radioState.paddleOrientation == NORMAL) ? ditdah(DIT) : ditdah(DAH);
                break;
            case BTN_PRESSED_PADDLE_RIGHT :
                buttonPressed = BTN_PRESSED_NONE;
                (radioState.paddleOrientation == NORMAL) ? ditdah(DAH) : ditdah(DIT);
                break;
            }
        }
    }
}

/*
 * This routine will handle response to the led encoder being rotated.
 */
static void handle_led_encoder(void)
{
    uint8_t led_index;

    if (GPIO_getInputPinValue(LED_ENCODER_A) != GPIO_getInputPinValue(LED_ENCODER_B))
    {
        Menu_SelectMove(ENCODER_CW);
    } else {
        Menu_SelectMove(ENCODER_CCW);
    }
    // menuSelectedIndex now contains the index to the new selected menu
    // now get led index associated with the new menu index
    led_index = Menu_GetSelectedLedIndex();
    radioState.ledIndex = led_index;
    // now send this to the LED shifter to update the LEDs
    updateLEDShifter(led_index);
    // now update LCD status field to show current menu option
    updateLCD_status();
}
/*
 * This routine will handle response to the menu option encoder being rotated.
 */
static void handle_menu_encoder(void)
{
    const MenuItem_t *item;
    uint8_t led_index;

    if (GPIO_getInputPinValue(MENU_ENCODER_A) != GPIO_getInputPinValue(MENU_ENCODER_B))
    {
        Option_SelectMove(ENCODER_CW);
    } else {
        Option_SelectMove(ENCODER_CCW);
    }
    // now execute callback function to update HW
    item = &menuTable[menuSelectedIndex];
    if (item->action != NULL)
    {
        item->action(item, menuCurrentValue[menuSelectedIndex]);
    }
    // need to update shifter even though LED won't change; but since the band could
    // have changed, we need to update everything
    led_index = Menu_GetSelectedLedIndex();
    updateLEDShifter(led_index);
    // menuSelectedIndex now contains the index to the new selected option
    // now update LCD status field to show current menu option
    updateLCD_status();
}
/*
 * This routine will handle response to the menu option encoder switch being activated.
 */
static void handle_menu_encoder_switch(void)
{
     uint8_t value;

    if (menuSelectedIndex == MENU_PLAY_MEM)
    {
        // the play mem menu is selected and the encoder switch was pressed
        // so play the selected memory
        value = (uint8_t)menuCurrentValue[menuSelectedIndex];
        playCwMsg(value);
    }
    else if (menuSelectedIndex == MENU_RECORD_MEM)
    {
        // the play mem menu is selected and the encoder switch was pressed
        // so play the selected memory
        value = (uint8_t)menuCurrentValue[menuSelectedIndex];
        recordCwMsg(value);
    }
}


static void handle_tuner_encoder(void)
{
    uint8_t i = radioState.bandIndex;
    extern uint8_t encoderCWCount, encoderCCWCount;

    if (encoderCWCount != 0)
    {
        if (radioState.frequency[i] + radioState.freqMultiplier <= radioState.maxBandFreq)
            radioState.frequency[i] += radioState.freqMultiplier;
    } else if (encoderCCWCount != 0)  {
        if (radioState.frequency[i] - radioState.freqMultiplier >= radioState.minBandFreq)
            radioState.frequency[i] -= radioState.freqMultiplier;
    }
    encoderCWCount = encoderCCWCount = 0;
    // now update lcd frequency field; always show rx frequency only
    updateLCD_freq();
    setSI5351Freq(radioState.frequency[i]);
}


// This routine will update menu LEDs and the latching relays for the filters
// Sends '4' bytes out to a chain of cascaded 74HCT595s and latches once.
// data[3] = byte for filter latching relays - first chip
// data[2] = byte for the second chip in the chain (closest to MCU, SER pin)
// data[1] = byte for the third chip
// data[0] = byte for the fourth chip
// (byte order is reversed internally since the chain shifts "backwards")
static void updateLEDShifter( uint8_t index)
{
    uint8_t i;
    uint8_t data[4];
    //uint8_t data[3];
    uint32_t selectedLED = 0;
    extern RadioState_t radioState;

    selectedLED = (1UL << index);  // convert number to bit
    selectedLED = ~selectedLED;  // invert all bits to match HW implementation of turning on LED
    // construct bytes to send
    data[3] = 0; // don't change the LPF/HPF settings
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







