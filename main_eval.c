#include "driverlib.h"
#include "main.h"

/*
 * This program uses the MSP430FR2476 eval board to test the
 * button selector board.
 * 3 pins for the shifter - shiftClk, shiftData and registerClk
 * 6 pins for two encoders with switches
 * 6 pins for driving LCD
 * P1.1, P1.2 and P1.7 for shifter
 * P4.6, P2.1, P2.0 - for menu encoder
 * P3.2, P3.6, P4.2 for menu option encoder
 * P5.2, P5.1, P5.0, P4.7, P6.0, P3.3 for LCD
 */
// Define all I/O



// Track valid button press states globally
volatile uint8_t button_pressed_flag = 0;


int main(void) {

    WDT_A_hold(WDT_A_BASE);
    initClocks();
    init_switch_s1s2();
    initGPIO();

    init_spi_shift_register(); // Hardware setup from the previous step
    init_switch_s1s2();          // S1 and S2 and Timer_A3 debounce setup

    while (1)
    {
        if (button_pressed_flag)
        {
            button_pressed_flag = 0; // Reset event flag

            // Execute task: Send shifted data byte over eUSCI_B0
            send_byte_to_shift_register(data_to_send);
            delay_ms(12);
            send_byte_to_shift_register(0);

            // Increment or modify the test byte
            if ( data_to_send == SELECT_40M) {
                data_to_send = SELECT_2030M;
            } else if (data_to_send == SELECT_2030M) {
                data_to_send = SELECT_1517M;
            } else {
                data_to_send = SELECT_40M;
            }
        }
    }
}

void send_byte_to_shift_register(uint8_t data)
{
    // Wait until the hardware transmit buffer is empty and ready
    while (!EUSCI_B_SPI_getInterruptStatus(EUSCI_B0_BASE, EUSCI_B_SPI_TRANSMIT_INTERRUPT));

    // Send the byte into the hardware buffer (it will auto-shift out 8 bits in the background)
    EUSCI_B_SPI_transmitData(EUSCI_B0_BASE, data);

    // Block until the shift register inside the eUSCI_B0 completely finishes serialization
    while (EUSCI_B_SPI_isBusy(EUSCI_B0_BASE));

    // Pulse the P1.7 Latch pin (RCLK) to push data to the 74HCT595 parallel physical pins
    GPIO_setOutputHighOnPin(LATCH_PORT, LATCH_PIN);
    __delay_cycles(10); // Small delay to satisfy 74HCT595 timing minimums
    GPIO_setOutputLowOnPin(LATCH_PORT, LATCH_PIN);
}



// -------------------------------------------------------------------------
// PORT 4 INTERRUPT SERVICE ROUTINE (Initial Button Edge Detection)
// -------------------------------------------------------------------------
#pragma vector=PORT4_VECTOR
__interrupt void Port_4_ISR(void)
{
    // Check if the interrupt was caused by the P4.0 switch pin
    if (GPIO_getInterruptStatus(GPIO_PORT_P4, GPIO_PIN0) == GPIO_PIN0)
    {
        // Temporarily disable the P4.0 interrupt to shield against mechanical bounce noise
        GPIO_disableInterrupt(GPIO_PORT_P4, GPIO_PIN0);
        GPIO_clearInterrupt(GPIO_PORT_P4, GPIO_PIN0);

        // Clear and restart Timer_A3 to begin the 15ms verification window
        Timer_A_clearTimerInterrupt(TIMER_A3_BASE);
        Timer_A_startCounter(TIMER_A3_BASE, TIMER_A_UP_MODE);
    }
}

// -------------------------------------------------------------------------
// TIMER_A3 CCR0 INTERRUPT SERVICE ROUTINE (Debounce Verification)
// -------------------------------------------------------------------------
#pragma vector=TIMER3_A0_VECTOR
__interrupt void Timer_A3_CCR0_ISR(void)
{
    // Halt Timer_A3 since our verification check is happening now
    Timer_A_stop(TIMER_A3_BASE);
    Timer_A_clear(TIMER_A3_BASE);

    // Sample the pin. If it remains LOW (0), it is a stable, intended press.
    if (GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN0) == GPIO_INPUT_PIN_LOW)
    {
        button_pressed_flag = 1; // Mark event for main program loop
    }
    else if (GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN3) == GPIO_INPUT_PIN_LOW)
    {
        button_pressed_flag = 1; // Mark event for main program loop
    }

    // Clear residual bounce noise, then re-arm the pin edge interrupt
    GPIO_clearInterrupt(GPIO_PORT_P4, GPIO_PIN0);
    GPIO_enableInterrupt(GPIO_PORT_P4, GPIO_PIN0);
    GPIO_clearInterrupt(GPIO_PORT_P2, GPIO_PIN3);
    GPIO_enableInterrupt(GPIO_PORT_P2, GPIO_PIN3);

}






