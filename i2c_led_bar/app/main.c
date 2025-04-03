#include <msp430.h> 
#include <stdint.h>

// Global variables
#define pattern_off 0x00
#define pattern_0 0x01
#define pattern_1 0x02
#define pattern_2 0x03
#define pattern_3 0x04
#define pattern_4 0x05
#define pattern_5 0x06
#define pattern_6 0x07
#define pattern_7 0x08

#define LED1 BIT1
#define LED2 BIT0
#define LED3 BIT7
#define LED4 BIT6
#define LED5 BIT0
#define LED6 BIT7
#define LED7 BIT6
#define LED8 BIT5
#define Data_LED BIT4

#define ADDRESS 0x02

volatile uint8_t led_state, step_pattern, counter_pattern_2, counter_pattern_4;
volatile uint8_t data_in = 0, read_in = 0;

volatile uint8_t step_pattern_0, step_pattern_1, step_pattern_2, step_pattern_3, step_pattern_4, step_pattern_5, step_pattern_6, step_pattern_7;
volatile uint8_t counter_pattern_2_0, counter_pattern_4_0;
volatile uint16_t transition = 32768;

// Helper function to update LEDs
void update_leds(uint8_t pattern)
{
    P1OUT &= ~(LED1 | LED2 | LED6 | LED7 | LED8);
    P2OUT &= ~(LED3 | LED4 | LED4);

    // Set LEDs based on the pattern
    P1OUT |= (pattern & 0x01) ? LED1 : 0;
    P1OUT |= (pattern & 0x02) ? LED2 : 0;
    P2OUT |= (pattern & 0x04) ? LED3 : 0;
    P2OUT |= (pattern & 0x08) ? LED4 : 0;
    P2OUT |= (pattern & 0x10) ? LED5 : 0;
    P1OUT |= (pattern & 0x20) ? LED6 : 0;
    P1OUT |= (pattern & 0x40) ? LED7 : 0;
    P1OUT |= (pattern & 0x80) ? LED8 : 0;
}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;  // Stop watchdog timer

    //---------------- Configure UCB0 I2C ----------------

    // Configure P1.2 (SDA) and P1.3 (SCL) for I2C on MSP430FR2310
    P1SEL0 |= BIT2 | BIT3;
    P1SEL1 &= ~(BIT2 | BIT3);

    // LED box configuration
    P1OUT |= LED1 | LED2 | LED6 | LED7 | LED8;
    P2OUT |= LED3 | LED4 | LED4;
    P1OUT &= ~(LED1 | LED2 | LED6 | LED7 | LED8);
    P2OUT &= ~(LED3 | LED4 | LED4);

    // Data LED
    P1DIR |= Data_LED;
    P1OUT &= ~Data_LED;

    // Configure Timers
    TB0CTL |= TBCLR; // Clear the timer
    TB0CTL |= TBSSEL__ACLK; // Use ACLK for timer
    TB0CTL |= MC__UP; // Up mode (timer counts up)

    TB0CCR0 = transition; // Timer compare register 0
    TB0CCTL0 &= ~CCIFG;   // Clear interrupt flag
    TB0CCTL0 |= CCIE;     // Enable CCR0 interrupt

    // I2C Configuration
    UCB0CTLW0 = UCSWRST;                 // Put eUSCI in reset
    UCB0CTLW0 |= UCMODE_3 | UCSYNC;      // I2C mode, synchronous mode
    UCB0I2COA0 = ADDRESS | UCOAEN;       // Set slave address and enable
    UCB0CTLW0 &= ~UCSWRST;               // Release eUSCI from reset
    UCB0IE |= UCRXIE0;                   // Enable receive interrupt

    PM5CTL0 &= ~LOCKLPM5; // Disable high-impedance mode
    __bis_SR_register(GIE);  // Enable global interrupts

    // Main loop does nothing, just handles interrupts
    while(1)
    {

    }

    return 0;
}

// Timer B0 interrupt
#pragma vector = TIMER0_B0_VECTOR
__interrupt void ISR_TB0_CCR0(void)
{
    uint8_t pattern = 0;

    switch (led_state)
    {
        case pattern_off:
            pattern = 0x00;
            break;

        case pattern_0:
            pattern = 0x55; // Alternating LEDs: 10101010
            step_pattern_0++;
            if (step_pattern_0 == 8) step_pattern_0 = 0;  // Reset after 8 steps
            break;

        case pattern_1:
            pattern = (step_pattern_1++ % 2 == 0) ? 0x55 : 0xAA; // Alternates between 10101010 and 01010101
            if (step_pattern_1 == 8) step_pattern_1 = 0;
            break;

        case pattern_2:
            pattern = counter_pattern_2_0++;
            if (counter_pattern_2_0 > 255) counter_pattern_2_0 = 0;
            break;

        case pattern_3:
            switch (step_pattern_3) {
                case 0: pattern = 0x18; break;
                case 1: pattern = 0x24; break;
                case 2: pattern = 0x42; break;
                case 3: pattern = 0x81; break;
                case 4: pattern = 0x42; break;
                case 5: pattern = 0x24; break;
                case 6: pattern = 0x18; break;
            }
            step_pattern_3 = (step_pattern_3 + 1) % 6;
            break;

        case pattern_4:
            pattern = counter_pattern_4_0--;
            if (counter_pattern_4_0 == 0) counter_pattern_4_0 = 255;
            break;

        case pattern_5:
            pattern = (1 << step_pattern_5);
            step_pattern_5 = (step_pattern_5 + 1) % 8;
            break;

        case pattern_6:
            pattern = (0xFF & ~(1 << step_pattern_6));
            step_pattern_6 = (step_pattern_6 + 1) % 8;
            break;

        case pattern_7:
            pattern = (1 << (step_pattern_7 + 1)) - 1;
            step_pattern_7 = (step_pattern_7 + 1) % 8;
            break;
    }

    update_leds(pattern);
    TB0CCTL0 &= ~CCIFG; // Clear the interrupt flag
}

// I2C RX interrupt
#pragma vector=USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
{
    read_in = UCB0RXBUF;

    if (read_in == led_state)
    {
        // Reset the pattern
        switch (read_in)
        {
            case 0x00: led_state = pattern_off; break;
            case 0x01: led_state = pattern_0; step_pattern_0 = 0; break;
            case 0x02: led_state = pattern_1; step_pattern_1 = 0; break;
            case 0x03: led_state = pattern_2; counter_pattern_2_0 = 0; break;
            case 0x04: led_state = pattern_3; step_pattern_3 = 0; break;
            case 0x05: led_state = pattern_4; counter_pattern_4_0 = 255; break;
            case 0x06: led_state = pattern_5; step_pattern_5 = 0; break;
            case 0x07: led_state = pattern_6; step_pattern_6 = 0; break;
            case 0x08: led_state = pattern_7; step_pattern_7 = 0; break;
        }
    }
    else
    {
        // Save the current state of the active pattern before switching
        switch (led_state) {
            case pattern_0: step_pattern_0 = step_pattern_0; break;
            case pattern_1: step_pattern_1 = step_pattern_1; break;
            case pattern_2: counter_pattern_2_0 = counter_pattern_2_0; break;
            case pattern_3: step_pattern_3 = step_pattern_3; break;
            case pattern_4: counter_pattern_4_0 = counter_pattern_4_0; break;
            case pattern_5: step_pattern_5 = step_pattern_5; break;
            case pattern_6: step_pattern_6 = step_pattern_6; break;
            case pattern_7: step_pattern_7 = step_pattern_7; break;
            default: break;
        }

        // Select the new pattern
        switch (read_in)
        {
            case 0x00: led_state = pattern_off; break;
            case 0x01: led_state = pattern_0; break;
            case 0x02: led_state = pattern_1; break;
            case 0x03: led_state = pattern_2; break;
            case 0x04: led_state = pattern_3; break;
            case 0x05: led_state = pattern_4; break;
            case 0x06: led_state = pattern_5; break;
            case 0x07: led_state = pattern_6; break;
            case 0x08: led_state = pattern_7; break;
        }
    }

}
