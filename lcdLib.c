#include "driverlib.h"
#include "lcdLib.h"
//#include "main.h"
#include <stdlib.h>
//#include <math.h>
#include "menu_data.h"
#include "radio_state.h"


static void lcdTriggerEN(void);
static void lcdWriteData(uint8_t *);
static void lcdWriteCmd(uint8_t);
static void setData(uint8_t);
static char *number_to_string(uint32_t);



#define STATUS_FIELD 0x40
#define STATUS_FIELD_WIDTH 12
#define FREQ_FIELD 0x00
#define FREQ_FIELD_WIDTH 12

// cursor positions are based on 30M, 20M, 17M and 15M bands
// for 40M, decrement all by 1
#define CURSOR_10 0x08
#define CURSOR_100 0x07
#define CURSOR_1K 0x06
#define CURSOR_10K 0x05

// Commands
#define CLEAR   0x01


#define BUFFER_SIZE 12
static char buffer[BUFFER_SIZE];  /* must be static to be able to return it */
/*
static char freqBuffer[16];
static char batVoltBuffer[8];
static char cwSpeedBuffer[8];
static char ritStateBuffer[16];
*/

static const LcdField_t fieldStatus = { STATUS_FIELD, STATUS_FIELD_WIDTH };
static const LcdField_t fieldFreq   = { FREQ_FIELD, FREQ_FIELD_WIDTH };


void lcdInit() {
	delay_ms(100);
	// Wait for 100ms after power is applied.

	setData(0x03);  // Start LCD (send 0x03)

	lcdTriggerEN(); // Send 0x03 3 times at 5ms then 100 us
	delay_ms(5);
	lcdTriggerEN();
	delay_ms(5);
	lcdTriggerEN();
	delay_ms(5);

	setData(0x02); // Switch to 4-bit mode
	lcdTriggerEN();
	delay_ms(5);

	lcdWriteCmd(0x28); // 4-bit, 2 line, 5x8
	lcdWriteCmd(0x08); // Instruction Flow
	lcdWriteCmd(0x01); // Clear LCD
	lcdWriteCmd(0x06); // Auto-Increment
	//lcdWriteCmd(0x0E); // Display On, Cursor On, No blink
	lcdWriteCmd(0x0C); // Display On, Cursor Off, No blink
}

static void lcdTriggerEN(void) {
    GPIO_setOutputHighOnPin(LCD_CLK);  // toggle clock (enable) bit
    delay_us(20);
    GPIO_setOutputLowOnPin(LCD_CLK);
}

static void lcdWriteData(uint8_t *data) {
    uint8_t i = 0;
    while (data[i] != '\0') {
        GPIO_setOutputHighOnPin(LCD_RS); // Set RS to data
        setData(data[i] >> 4); // Upper nibble
        lcdTriggerEN();
        setData(data[i]); // Lower nibble
        lcdTriggerEN();
        delay_us(50); // Delay > 47 us
        i++;
    }
}

static void lcdWriteCmd(uint8_t cmd) {
    GPIO_setOutputLowOnPin(LCD_RS);
    setData(cmd >> 4);
    lcdTriggerEN();
    setData(cmd);
    lcdTriggerEN();
    if (cmd == CLEAR || cmd == 0x02 /* return home */)
        delay_ms(2);
    else
        delay_us(50);
}


// This function will take a 4 bit data nibble and split it such that the bits
// in position 2,3 are shifted up to positions 5,6 to match the mapping of the GPIO
// to the LCD module.  It will then set the P2OUT ports to the resulting value
static void setData(uint8_t data)
{
    (data & 0x01) ? (GPIO_setOutputHighOnPin(LCD_D4)) : (GPIO_setOutputLowOnPin(LCD_D4));
    (data & 0x02) ? (GPIO_setOutputHighOnPin(LCD_D5)) : (GPIO_setOutputLowOnPin(LCD_D5));
    (data & 0x04) ? (GPIO_setOutputHighOnPin(LCD_D6)) : (GPIO_setOutputLowOnPin(LCD_D6));
    (data & 0x08) ? (GPIO_setOutputHighOnPin(LCD_D7)) : (GPIO_setOutputLowOnPin(LCD_D7));
}

static char *number_to_string(uint32_t number)
{
    char *p;
    uint32_t digit;

    p = &buffer[BUFFER_SIZE - 1];
    *p-- = '\0';  /* end of string */
    do {
        digit = number % 10;
        *p-- = '0' + digit;
        number /= 10;
    } while (number > 0);
    return p+1;  /* first digit might not be at the start of the buffer */
}

// routine to move cursor for frequency readout; always on line 1
//
void moveFreqCursor(uint8_t index)
{
    uint8_t address;
    const uint8_t cursorAddr[] = {CURSOR_10, CURSOR_100, CURSOR_1K, CURSOR_10K};

    address = cursorAddr[index];
    if ( radioState.bandIndex == BAND_40M )
        address--;

    lcdWriteCmd(MOVE_CURSOR + address);
}
/*****************
 * All functions above are the low level LCD routines
 */

/*******************************
// routine to display frequency
void updateDisplay(uint8_t field)
{
    char *result;
    extern uint8_t selectedBand;
    extern uint32_t si5351FreqOut;
    extern uint8_t selectedFilter;
    extern uint8_t selectedSideband;
    extern uint16_t batteryVoltage;
    extern uint8_t selectedMenuFunction;
    extern int16_t ritOffset;
    extern uint8_t ritState;
    extern uint8_t audioState;
    extern uint8_t receiveMode;
    extern uint16_t qskDelay;
    extern uint8_t paddleOrientation;
    extern uint8_t selectedMem;
    extern uint8_t iambicMode;
    uint8_t i;
    float z;
    uint32_t batV;


    //lcdClear();

    // display frequency
    //lcdSetInt(si5351FreqOut, 0, 0);
    if ( field == FREQ_DISPLAY)  // update frequency field
    {
        result = number_to_string(si5351FreqOut);
        if (selectedBand == BAND_40M)
        {
            freqBuffer[0] = *result++;
            freqBuffer[1] = '.';
            freqBuffer[2] = *result++;
            freqBuffer[3] = *result++;
            freqBuffer[4] = *result++;
            freqBuffer[5] = '.';
            freqBuffer[6] = *result++;
            freqBuffer[7] = *result++;
            freqBuffer[8] = *result++;
            freqBuffer[9] = '\0';
        } else {
            freqBuffer[0] = *result++;
            freqBuffer[1] = *result++;
            freqBuffer[2] = '.';
            freqBuffer[3] = *result++;
            freqBuffer[4] = *result++;
            freqBuffer[5] = *result++;
            freqBuffer[6] = '.';
            freqBuffer[7] = *result++;
            freqBuffer[8] = *result++;
            freqBuffer[9] = *result++;
            freqBuffer[10] = '\0';
        }
        lcdSetText("          ",0,0);  // clear field
        lcdSetText(freqBuffer,0,0);
    }

    else if (field == BAND_DISPLAY) // update band field
    {
        // display band
        lcdSetText("   ",0xD,0); // clear field
        switch (selectedBand)
        {
        case  BAND_40M :
            lcdSetText("40M",BAND_FIELD);  // put band info at position 13 (0xD) on first row
            break;
        case  BAND_30M :
            lcdSetText("30M",BAND_FIELD);  // put band info at position 13 (0xD) on first row
            break;
        case  BAND_20M :
            lcdSetText("20M",BAND_FIELD);  // put band info at position 13 (0xD) on first row
            break;
        case  BAND_17M :
            lcdSetText("17M",BAND_FIELD);  // put band info at position 13 (0xD) on first row
            break;
        case  BAND_15M :
            lcdSetText("15M",BAND_FIELD);  // put band info at position 13 (0xD) on first row
            break;
        default :
            break;
        }
    }

    else if (field == MENU_DISPLAY)
    {
        lcdSetText("            ",0,1);
        // display menu function if RIT is not enabled
        if (ritState == ENABLED)
        {
            lcdSetText("RIT: ",0,1);
            result = number_to_string((uint32_t)(abs(ritOffset)));
            (ritOffset < 0) ? (ritStateBuffer[0] = '-') : (ritStateBuffer[0] = '+');
            i = 1;
            while (*result != '\0')
                ritStateBuffer[i++] = *result++;
            ritStateBuffer[i] = '\0';
            lcdSetText(ritStateBuffer,5,1);
        }
        else if (audioState == MUTE)
        {
            lcdSetText("MUTE",0,1);
        }
        else
        {
            switch (selectedMenuFunction)
            {
            case MENU_FUNCTION_BATVOLTAGE :
                getBatteryVoltage();
                // assume external resistor divider is 0.1 and full scale is 1.5V
                // ((batteryVoltage * 1.5/0.1) * 65536)/4096 = batteryVoltage * 240
                // Actual resistor divider is about 0.098, not 0.1, so 245 is used instead of 240
                //z = ((float)batteryVoltage/4096.0)*1.5/0.098;
                //batV = (uint32_t)(round(z*10.0));
                //setBatVoltText(batV);
                // claude change to remove dependence on math.h
                // batV = round(batteryVoltage * 1.5 * 10 / (0.098 * 4096))
                //      = round(batteryVoltage * 15000 / 401.408)  -> scale to avoid float
                batV = ((uint32_t)batteryVoltage * 15300 + 200704) / 401408;
                break;
            case MENU_FUNCTION_CWSPEED :
                setCWSpeedText();
                break;
            case MENU_FUNCTION_QSK_DELAY :
                lcdSetText("QSK: ",0,1);
                result = number_to_string((uint32_t)qskDelay);
                i = 0;
                while (*result != '\0')  // use ritStateBuffer to hold qsk delay string
                    ritStateBuffer[i++] = *result++;
                ritStateBuffer[i] = '\0';
                lcdSetText(ritStateBuffer,5,1);
                break;
            case MENU_FUNCTION_PADDLE_ORIENTATION :
                lcdSetText("KEY: ",0,1);
                (paddleOrientation == PADDLE_DAH_DIT) ? lcdSetText("DAH_DIT", 5,1) : lcdSetText("DIT_DAH", 5,1);
                break;
            case MENU_FUNCTION_RECORD_MEMORY :
                if (selectedMem == MEM1)
                    lcdSetText("RECORD MEM1",0,1);
                else if (selectedMem == MEM2)
                    lcdSetText("RECORD MEM2",0,1);
                else
                    lcdSetText("RECORD MEM3",0,1);
                break;
            case MENU_FUNCTION_IAMBIC_MODE :
                if (iambicMode == IAMBIC_MODE_A)
                    lcdSetText("IAMBIC A",0,1);
                else if (iambicMode == IAMBIC_MODE_B)
                    lcdSetText("IAMBIC B",0,1);
                else
                    lcdSetText("ULTIMATIC",0,1);
                break;
            default :
                break;
            }
        }
    }
    else if (field == MODE_DISPLAY)
    {
        lcdSetText("   ",0xD,1);
        // display filter selection
        if (receiveMode == RXMODE_CW)
            lcdSetText(" CW",0xD,1);
        else if (receiveMode == RXMODE_USB)
            lcdSetText("USB",0xD,1);
        else
            lcdSetText("LSB",0xD,1);
    }
    else if (field == PLAY_MEM_DISPLAY)
    {
        lcdSetText("            ",0,1);
        if (selectedMem == MEM1)
            lcdSetText("PLAY MEM1",0,1);
        else if (selectedMem == MEM2)
            lcdSetText("PLAY MEM2",0,1);
        else
            lcdSetText("PLAY MEM3",0,1);
    }
    moveFreqCursor();
}
*******************/
void updateLCD_status(void)
{
    const char *result;
    const char *suffix = NULL;

    if (menuTable[menuSelectedIndex].type == MENU_TYPE_RANGE)
    {
        result = number_to_string((uint32_t)menuCurrentValue[menuSelectedIndex]);
        suffix = menuTable[menuSelectedIndex].def.range.unitSuffix; // adjust to match your actual field name
    }
    else
    {
        result = menuTable[menuSelectedIndex].def.list.options[(menuCurrentValue[menuSelectedIndex])];
    }

    LCD_WriteField(&fieldStatus, result, suffix);
}

void LCD_WriteField(const LcdField_t *field, const char *text, const char *suffix)
{
    uint8_t buf[16 + 1];
    uint8_t len = (uint8_t)strlen(text);

    if (len > field->fieldWidth)
    {
        len = field->fieldWidth;
    }
    memcpy(buf, text, len);

    if (suffix != NULL && len < field->fieldWidth)
    {
        uint8_t suffixLen = (uint8_t)strlen(suffix);
        uint8_t room = field->fieldWidth - len;
        if (suffixLen > room)
        {
            suffixLen = room;
        }
        memcpy(&buf[len], suffix, suffixLen);
        len += suffixLen;
    }

    if (len < field->fieldWidth)
    {
        memset(&buf[len], ' ', field->fieldWidth - len);
    }
    buf[field->fieldWidth] = '\0';

    lcdWriteCmd(0x80 | field->baseAddr);
    lcdWriteData(buf);
}

void updateLCD_freq(void)
{
    char *result;
    char freqBuffer[16];
    const char *suffix = NULL;

    result = number_to_string(radioState.frequency[radioState.bandIndex]);
    if (radioState.bandIndex == BAND_40M)
    {
        freqBuffer[0] = *result++;
        freqBuffer[1] = '.';
        freqBuffer[2] = *result++;
        freqBuffer[3] = *result++;
        freqBuffer[4] = *result++;
        freqBuffer[5] = '.';
        freqBuffer[6] = *result++;
        freqBuffer[7] = *result++;
        freqBuffer[8] = *result++;
        freqBuffer[9] = '\0';
    } else {
        freqBuffer[0] = *result++;
        freqBuffer[1] = *result++;
        freqBuffer[2] = '.';
        freqBuffer[3] = *result++;
        freqBuffer[4] = *result++;
        freqBuffer[5] = *result++;
        freqBuffer[6] = '.';
        freqBuffer[7] = *result++;
        freqBuffer[8] = *result++;
        freqBuffer[9] = *result++;
        freqBuffer[10] = '\0';
    }
    LCD_WriteField(&fieldFreq, freqBuffer, suffix);
}
/*

  // This routine will take the battery voltage and convert it to text for the LCD
void setBatVoltText(uint32_t result)
{
   char *txt;

    // now convert to string
    txt = number_to_string(result);

    batVoltBuffer[0] = *txt++;
    if ( result < 100)
    {
        batVoltBuffer[1] = '.';
        batVoltBuffer[2] = *txt++;
        batVoltBuffer[3] = ' ';
        batVoltBuffer[4] = 'V';
        batVoltBuffer[5] = '\0';
    }
    else
    {
        batVoltBuffer[1] = *txt++;
        batVoltBuffer[2] = '.';
        batVoltBuffer[3] = *txt++;
        batVoltBuffer[4] = ' ';
        batVoltBuffer[5] = 'V';
        batVoltBuffer[6] = '\0';
    }
    lcdSetText(batVoltBuffer,0);
}

// This routine will take the battery voltage and convert it to text for the LCD
void setCWSpeedText(void)
{
    extern uint8_t wpm;
    char *txt;


    lcdSetText("CWSPD: ",0,1);

    // now convert to string
    txt = number_to_string((uint32_t)wpm);

    cwSpeedBuffer[0] = *txt++;
    if (wpm > 9)
    {
        cwSpeedBuffer[1] = *txt++;
        cwSpeedBuffer[2] = '\0';
    }  else {
        cwSpeedBuffer[1] = '\0';
    }
    lcdSetText(cwSpeedBuffer,7,1);
}
*/
