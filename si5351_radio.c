#include <msp430.h>
#include "driverlib.h"
#include <stdint.h>
#include "si5351.h"
#include "radio_state.h"

#define SI5351_ADDRESS          0x60

// Si5351 registers
#define DEVICE_STATUS           0
#define REG_OUTPUT_ENABLE       3
#define PLL_INPUT_SRC           15
#define REG_CLK0_CTRL           16
#define REG_CLK1_CTRL           17
#define REG_CLK2_CTRL           18
#define CLK_DISABLE_STATE       24
#define REG_PLLA_PARAMETERS     26
#define REG_MS0_PARAMETERS      42
#define REG_MS1_PARAMETERS      50
#define SSEN                    149
#define PLL_RESET               177
#define REG_CLK0_PHOFF          165
#define REG_CLK1_PHOFF          166
#define XTAL_LOAD_CAP           183

#define RECEIVE_MODE 0
#define CW 0
#define CWR 1

/*
 * Crystal load capacitance register (183).
 * Bits 7:6 select the load, bits 5:0 must be 010010.
 *   6 pF = 0x52,  8 pF = 0x92,  10 pF = 0xD2 (chip default)
 * Set this to match the load capacitance in YOUR crystal's datasheet.
 * (The original code wrote 0x12, which puts the reserved value 00 in
 * bits 7:6.)
 */
#define XTAL_CL_VALUE           0xD2

//#define XTAL_NOMINAL_HZ         25000000UL
#define XTAL_NOMINAL_HZ         24999503UL
#define MAX_PHASE_OFFSET        127UL      // phase offset registers are 7 bits

#define delay_us(x)     __delay_cycles((long) x * 8)

#define I2C_RECEIVE 0
#define I2C_SEND 1

// Longest burst is 16 data bytes (MS0 + MS1 registers), plus 1 register byte
#define I2C_MAX_BURST           16

// 100 kbps works today. The Si5351 supports 400 kbps if your pull-ups and
// wiring allow it: EUSCI_B_I2C_SET_DATA_RATE_400KBPS
#define I2C_DATA_RATE           EUSCI_B_I2C_SET_DATA_RATE_100KBPS

static uint8_t RXData[4];  // used by interrupt routine to hold received data
static uint8_t TXData[I2C_MAX_BURST + 1];
static uint8_t byteCount;
static uint8_t I2CMode;    // indicate whether I2C is sending or receiving

// Which mode the eUSCI is currently configured for; 0xFF = not configured.
// The block is only re-initialized when the mode actually changes.
static uint8_t i2cConfigured = 0xFF;

static void i2cConfigure(uint8_t mode);
static void i2cTransmit(uint8_t count);
static void i2cSetRegPointer(uint8_t reg);
static void i2cSendRegister(uint8_t reg, uint8_t data);
static void i2cSendBurst(uint8_t reg, const uint8_t *data, uint8_t len);
static void i2cReceiveData(void);
static uint32_t CalcRegisters(const uint32_t, uint8_t *);

// ---- calibration ----------------------------------------------------------
// Actual crystal frequency in Hz. Start at 25 MHz, then set it from your
// measurement: xtal_hz = 25000000 * (1 + error), where error is positive if
// the crystal (and therefore every output) runs HIGH.
static uint32_t xtal_hz = XTAL_NOMINAL_HZ;

// Last output multisynth divider written. Used to decide whether the MS
// registers, phase offsets, and PLL reset must be redone.
static uint32_t lastD = 0;

void si5351_set_xtal_hz(uint32_t hz)
{
    xtal_hz = hz;
    lastD = 0;              // force a full reload on the next frequency set
}

/*
 * Sets the si5351 clk0 and clk1 outputs for the receive frequency
 * (quadrature). Returns 0 on success, -1 if the frequency needs a
 * phase offset larger than the 7-bit register allows (below about 4.7 MHz).
 *
 * NOTE: change the prototype in si5351.h from void to int.
 *
 * If d is unchanged from the last call, only the PLL feedback registers are
 * updated. There is no PLL reset, so tuning within a band is glitch-free.
 */
int setSI5351Freq(uint32_t freq)
{
    uint8_t regs[16];
    uint8_t i;
    static int lastSideband = -1;

    // adjust frequency as needed
    if (radioState.txMode == RECEIVE_MODE)
    {
        freq += radioState.ritOffset;
        if (radioState.modeIndex == CW || radioState.modeIndex == CWR)
            (radioState.selectedSideband == LOWER_SIDEBAND) ? (freq += radioState.sidetoneFreq) : (freq -= radioState.sidetoneFreq);
    }
    else
    {
        if ((freq + radioState.xitOffset <= radioState.maxBandFreq) && (freq + radioState.xitOffset >= radioState.minBandFreq))
        {
            freq += radioState.xitOffset;
        }
    }
    uint32_t d = CalcRegisters(freq, regs);

    if (d > MAX_PHASE_OFFSET)
        return -1;

    // PLLA feedback multisynth (always updated): 8 bytes, one transaction
    i2cSendBurst(REG_PLLA_PARAMETERS, regs, 8);


    if (d != lastD || (int)radioState.selectedSideband != lastSideband)
    {
        // MS0 (regs 42-49) and MS1 (regs 50-57) are contiguous and use the
        // same divider, so send both as one 16-byte transaction.
        uint8_t ms[16];
        for (i = 0; i < 8; i++)
            ms[i] = ms[8 + i] = regs[8 + i];
        i2cSendBurst(REG_MS0_PARAMETERS, ms, 16);

        // CLK0_PHOFF (165) and CLK1_PHOFF (166) are also contiguous.
        // Swap the two bytes to change the phase relationship (usb/lsb).
        uint8_t ph[2];
        if (radioState.selectedSideband == LOWER_SIDEBAND) {
            ph[0] = (uint8_t)d;     // CLK0 phase offset
            ph[1] = 0;              // CLK1 phase offset
        } else {
            ph[1] = (uint8_t)d;     // CLK1 phase offset
            ph[0] = 0;              // CLK0 phase offset
        }
        i2cSendBurst(REG_CLK0_PHOFF, ph, 2);

        // Phase offsets take effect on a PLL reset
        delay_us(500);
        i2cSendRegister(PLL_RESET, 0x20);   // reset PLLA

        lastD = d;
        lastSideband = (int)radioState.selectedSideband;
    }
    return 0;
}

// Disable spread spectrum. Call once from initialize_si5351().
void si5351_disable_spread_spectrum(void)
{
    i2cSendRegister(SSEN, 0);
}

void initialize_si5351(void)
{
    // Wait for SYS_INIT to clear
    do {
        i2cSetRegPointer(DEVICE_STATUS);    // point at the status register
        RXData[0] = 0;
        i2cReceiveData();
    } while (RXData[0] & 0x80);

    i2cSendRegister(REG_OUTPUT_ENABLE, 0xFF);   // disable all outputs

    i2cSendRegister(REG_CLK0_CTRL, 0x80);       // power down CLK0-2
    i2cSendRegister(REG_CLK1_CTRL, 0x80);
    i2cSendRegister(REG_CLK2_CTRL, 0x80);

    i2cSendRegister(PLL_INPUT_SRC, 0x00);       // XTAL is the PLL reference
    i2cSendRegister(CLK_DISABLE_STATE, 0x00);   // outputs low when disabled

    si5351_disable_spread_spectrum();

    // CLK0/CLK1: powered up, PLLA, integer mode, not inverted,
    // source = own multisynth, 8 mA drive
    i2cSendRegister(REG_CLK0_CTRL, 0x4F);
    i2cSendRegister(REG_CLK1_CTRL, 0x4F);
    i2cSendRegister(REG_CLK2_CTRL, 0xCF);       // CLK2 stays powered down

    i2cSendRegister(XTAL_LOAD_CAP, XTAL_CL_VALUE);

    i2cSendRegister(REG_OUTPUT_ENABLE, 0xFC);   // enable CLK0 and CLK1
    lastD = 0;                                  // force full load on next set
}

/*
 * Computes the register values for PLLA feedback (regs[0..7]) and the
 * output multisynth (regs[8..15]). Returns the output divider d, which is
 * also the phase offset value for 90 degrees.
 * Integer math only (64-bit intermediate), no floating point.
 */
static uint32_t CalcRegisters(const uint32_t fout, uint8_t *regs)
{
    uint32_t d = 4;
    uint32_t msx_p1 = 0;
    int msx_divby4 = 0;
    int rx_div = 0;
    uint32_t r = 1;

    if (fout > 150000000UL)
        msx_divby4 = 0x0C;                       // MSx_DIVBY4[1:0] = 0b11
    else if (fout < 292969UL)                    // low frequency: use R divider
    {
        int rd = 0;
        while ((r < 128) && (r * fout < 292969UL))
        {
            r <<= 1;
            rd++;
        }
        rx_div = rd << 4;

        d = 600000000UL / (r * fout);
        if (d % 2)
            d++;
        if (d * r * fout < 600000000UL)
            d += 2;
    }
    else                                         // 292969 Hz to 150 MHz
    {
        d = 600000000UL / fout;
        if (d < 6)
            d = 6;
        else if (d % 2)
            d++;

        if (d * fout < 600000000UL)
            d += 2;
    }
    msx_p1 = 128UL * d - 512;

    uint32_t fvco = d * r * fout;

    // Feedback multisynth: fvco / xtal_hz = a + b/c
    uint32_t a   = fvco / xtal_hz;
    uint32_t rem = fvco % xtal_hz;
    uint32_t c   = 1048575UL;
    uint32_t b   = (uint32_t)(((uint64_t)rem * c + xtal_hz / 2) / xtal_hz);

    if (b >= c)                 // rounded up to the next integer
    {
        a++;
        b = 0;
    }
    if (b == 0)
        c = 1;

    uint32_t msnx_p1 = 128UL * a + (128UL * b) / c - 512;
    uint32_t msnx_p2 = 128UL * b - c * ((128UL * b) / c);
    uint32_t msnx_p3 = c;

    // Feedback multisynth register values
    regs[0] = (msnx_p3 >> 8) & 0xFF;
    regs[1] = msnx_p3 & 0xFF;
    regs[2] = (msnx_p1 >> 16) & 0x03;
    regs[3] = (msnx_p1 >> 8) & 0xFF;
    regs[4] = msnx_p1 & 0xFF;
    regs[5] = ((msnx_p3 >> 12) & 0xF0) + ((msnx_p2 >> 16) & 0x0F);
    regs[6] = (msnx_p2 >> 8) & 0xFF;
    regs[7] = msnx_p2 & 0xFF;

    // Output multisynth and R divider register values (e = 0, f = 1)
    regs[8]  = 0;
    regs[9]  = 1;
    regs[10] = rx_div + msx_divby4 + ((msx_p1 >> 16) & 0x03);
    regs[11] = (msx_p1 >> 8) & 0xFF;
    regs[12] = msx_p1 & 0xFF;
    regs[13] = 0;
    regs[14] = 0;
    regs[15] = 0;

    return d;
}

/**************** I2C interrupt service routine ******************/
// Unchanged from your original.
#pragma vector=USCI_B0_VECTOR
__interrupt
void USCIB0_ISR(void)
{
    static uint8_t count = 0;
    switch(__even_in_range(UCB0IV,0x1E))
    {
    case 0x00: break;       // Vector 0: No interrupts break;
    case 0x02: break;       // Vector 2: ALIFG break;
    case 0x04:
        if (I2CMode == I2C_RECEIVE) {
            EUSCI_B_I2C_masterReceiveStart(EUSCI_B0_BASE);
        } else {
            EUSCI_B_I2C_masterSendStart(EUSCI_B0_BASE);
        }
        break;     // Vector 4: NACKIFG break;
    case 0x06: break;       // Vector 6: STT IFG break;
    case 0x08: break;       // Vector 8: STPIFG break;
    case 0x0a: break;       // Vector 10: RXIFG3 break;
    case 0x0c: break;       // Vector 14: TXIFG3 break;
    case 0x0e: break;       // Vector 16: RXIFG2 break;
    case 0x10: break;       // Vector 18: TXIFG2 break;
    case 0x12: break;       // Vector 20: RXIFG1 break;
    case 0x14: break;       // Vector 22: TXIFG1 break;
    case 0x16:
        RXData[count++] = EUSCI_B_I2C_masterReceiveSingle(EUSCI_B0_BASE);   // Get RX data
        if ( count == byteCount) {
            count = 0;
            __bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
        }
        break;     // Vector 24: RXIFG0 break;
    case 0x18:
        if (++count < byteCount)                    // Check TX byte counter
        {
            EUSCI_B_I2C_masterSendMultiByteNext(EUSCI_B0_BASE,TXData[count] );
        }
        else
        {
            EUSCI_B_I2C_masterSendMultiByteStop(EUSCI_B0_BASE);
            count = 0;
            __bic_SR_register_on_exit(LPM0_bits);// Exit LPM0
        }

        break;       // Vector 26: TXIFG0 break;
    case 0x1a: break;           // Vector 28: BCNTIFG break;
    case 0x1c: break;       // Vector 30: clock low timeout break;
    case 0x1e: break;       // Vector 32: 9th bit break;
    default: break;
    }
}

/**************** I2C setup ******************/
// Configures the eUSCI for send or receive. Does nothing if it is already
// configured for that mode, so back-to-back writes skip the setup entirely.
static void i2cConfigure(uint8_t mode)
{
    if (mode == i2cConfigured)
        return;

    EUSCI_B_I2C_initMasterParam param = {0};
    param.selectClockSource = EUSCI_B_I2C_CLOCKSOURCE_SMCLK;
    param.i2cClk = CS_getSMCLK();
    param.dataRate = I2C_DATA_RATE;

    if (mode == I2C_SEND) {
        param.byteCounterThreshold = 0;
        param.autoSTOPGeneration = EUSCI_B_I2C_NO_AUTO_STOP;
    } else {
        // only one receive is ever done here: a single status byte
        param.byteCounterThreshold = 1;
        param.autoSTOPGeneration = EUSCI_B_I2C_SEND_STOP_AUTOMATICALLY_ON_BYTECOUNT_THRESHOLD;
    }
    EUSCI_B_I2C_initMaster(EUSCI_B0_BASE, &param);

    EUSCI_B_I2C_setSlaveAddress(EUSCI_B0_BASE, SI5351_ADDRESS);
    EUSCI_B_I2C_setMode(EUSCI_B0_BASE, (mode == I2C_SEND) ?
                        EUSCI_B_I2C_TRANSMIT_MODE : EUSCI_B_I2C_RECEIVE_MODE);
    EUSCI_B_I2C_enable(EUSCI_B0_BASE);

    // Start from a clean interrupt state, then enable what this mode needs
    EUSCI_B_I2C_disableInterrupt(EUSCI_B0_BASE,
            EUSCI_B_I2C_TRANSMIT_INTERRUPT0 +
            EUSCI_B_I2C_RECEIVE_INTERRUPT0 +
            EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT +
            EUSCI_B_I2C_NAK_INTERRUPT);

    uint16_t ints = (mode == I2C_SEND) ?
            (EUSCI_B_I2C_TRANSMIT_INTERRUPT0 + EUSCI_B_I2C_NAK_INTERRUPT) :
            (EUSCI_B_I2C_RECEIVE_INTERRUPT0 + EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT +
             EUSCI_B_I2C_NAK_INTERRUPT);
    EUSCI_B_I2C_clearInterrupt(EUSCI_B0_BASE, ints);
    EUSCI_B_I2C_enableInterrupt(EUSCI_B0_BASE, ints);

    i2cConfigured = mode;
}

/**************** I2C send ******************/
// Sends the first 'count' bytes of TXData as one transaction
// (START, address, TXData[0..count-1], STOP), sleeping in LPM0 while the
// interrupt routine feeds the bytes.
static void i2cTransmit(uint8_t count)
{
    I2CMode = I2C_SEND;
    i2cConfigure(I2C_SEND);

    // make sure stop has been sent
    while (EUSCI_B_I2C_SENDING_STOP == EUSCI_B_I2C_masterIsStopSent
            (EUSCI_B0_BASE));

    byteCount = count;
    EUSCI_B_I2C_masterSendMultiByteStart(EUSCI_B0_BASE, TXData[0]);
    __bis_SR_register(LPM0_bits + GIE);         // sleep until the ISR is done

    // wait until transmission completes before returning
    while(EUSCI_B_I2C_isBusBusy(EUSCI_B0_BASE)) {;}
}

// Writes one register.
static void i2cSendRegister(uint8_t reg, uint8_t data)
{
    TXData[0] = reg;
    TXData[1] = data;
    i2cTransmit(2);
}

// Writes 'len' bytes (max I2C_MAX_BURST) to consecutive registers starting
// at 'reg'. The Si5351 auto-increments its register address.
static void i2cSendBurst(uint8_t reg, const uint8_t *data, uint8_t len)
{
    uint8_t i;

    TXData[0] = reg;
    for (i = 0; i < len; i++)
        TXData[1 + i] = data[i];
    i2cTransmit(len + 1);
}

// Sets the Si5351's register pointer (address byte only, no data) so that
// a following read starts from that register.
static void i2cSetRegPointer(uint8_t reg)
{
    TXData[0] = reg;
    i2cTransmit(1);
}

/*************** I2C Receive Command *****************/
// Reads one byte (the current register pointer) into RXData[0].
// Use i2cSetRegPointer() first to choose the register.
static void i2cReceiveData(void)
{
    I2CMode = I2C_RECEIVE;
    byteCount = 1;
    i2cConfigure(I2C_RECEIVE);

    // make sure stop has been sent
    while (EUSCI_B_I2C_SENDING_STOP == EUSCI_B_I2C_masterIsStopSent
            (EUSCI_B0_BASE));

    EUSCI_B_I2C_masterReceiveStart(EUSCI_B0_BASE);
    __bis_SR_register(LPM0_bits + GIE);         // sleep until the byte arrives
}
