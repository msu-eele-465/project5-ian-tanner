#include <msp430.h>
#include <stdint.h>
#include <math.h>

/**
 * main.c
 */

#define LCD_ADDRESS 0x01   // Address of the LCD MSP430FR2310
#define LED_ADDRESS 0X02   // Address of LED Bar MSP
#define TX_BYTES 5         // Number of bytes to transmit
#define REF_VOLTAGE 3.3    // ADC reference

// ADC Data
int window_size = 3;
int adc_results[10];
int sample_index = 0;
int samples_collected = 0;
int temperature_integer = 0;
int temperature_decimal = 0;

// I2C Data
volatile int tx_index = 0;
char tx_buffer[TX_BYTES] = {0, 0, 0, 0, 0}; // Default locked buffer

char led_buffer[9] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

volatile int led_index = 0;

void send_I2C_data()
{

    tx_index = 0; // Reset buffer index
    UCB0CTLW0 |= UCTR | UCTXSTT;  // Start condition, put master in transmit mode
    UCB0IE |= UCTXIE0; // Enable TX interrupt
}

void send_led_i2c()
{

    led_index = 0;
    UCB1CTLW0 |= UCTR | UCTXSTT;  // Start condition, put master in transmit mode
    UCB1IE |= UCTXIE1; // Enable TX interrupt

}

void start_ADC_conversion()
{

    ADCCTL0 |= ADCENC | ADCSC;

}

void get_temperature()
{

    int i;
    unsigned int total_adc_value = 0;
    for (i = 1; i <= window_size; i++)
    {

        total_adc_value += adc_results[i];

    }

    unsigned int average_adc_value = total_adc_value / window_size;
    float voltage = (average_adc_value / 4095.0) * REF_VOLTAGE;
    float temperature = -1481.96 + sqrt(2.1962e6 + ((1.8639 - voltage) / (3.88e-6)));
    temperature_integer = (int)temperature;
    temperature *= 10.0;
    temperature_decimal = (int)(temperature - temperature_integer);
    tx_buffer[2] = temperature_integer;
    tx_buffer[3] = temperature_decimal;
}

// Keypad data
// 2D Array, each array is a row, each item is a column.

char keyPad[][4] = {{'1', '2', '3', 'A'},  // Top Row
                    {'4', '5', '6', 'B'},
                    {'7', '8', '9', 'C'},
                    {'*', '0', '#', 'D'}}; // Bottom Row
/*                    ^              ^
 *                    |              |
 *                    Left Column    Right Column
 */

int column, row = 0;

char key_pressed = '\0';

char pass_code[] = "2659";
char input_code[] = "0000";

int mili_seconds_surpassed = 0;

int index = 0;  // Which index of the above input_code array we're in
int state = 0;  // State 0: Locked, State 1: Unlocking, State 2: Unlocked, State 3: Window Size Input, State 4: Pattern Input
int period = 0;

unsigned int transition = 32768;

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    //---------------- Configure ADC ---------------
    // Set P1.0 as ADC input
    P1SEL0 |= BIT0;
    P1SEL1 |= BIT0;

    ADCCTL0 &= ~ADCSHT;
    ADCCTL0 |= ADCSHT_2;
    ADCCTL0 |= ADCON;
    ADCCTL1 |= ADCSSEL_2;
    ADCCTL1 |= ADCSHP;
    ADCCTL2 &= ~ADCRES;
    ADCCTL2 |= ADCRES_2;
    ADCMCTL0 |= ADCINCH_0;
    ADCIE |= ADCIE0;
    //---------------- End Configure ADC ------------

    //---------------- Configure TB0 ----------------
    TB0CTL |= TBCLR;            // Clear TB0 timer and dividers
    TB0CTL |= TBSSEL__SMCLK;    // Select SMCLK as clock source
    TB0CTL |= MC__UP;            // Choose UP counting

    TB0CCR0 = 1000;             // TTB0CCR0 = 1000, since 1/MHz * 1000 = 1 ms
    TB0CCTL0 &= ~CCIFG;         // Clear CCR0 interrupt flag
    TB0CCTL0 |= CCIE;           // Enable interrupt vector for CCR0

    //---------------- End Configure TB0 ----------------

    //---------------- Configure P3 ----------------
    // Configure P3 for digital I/O
    P3SEL0 &= 0x00;
    P3SEL1 &= 0x00;

    P3DIR &= 0x0F;  // CLEARING bits 7 - 4, that way they are set to INPUT mode
    P3DIR |= 0X0F;  // SETTING bits 0 - 3, that way they are set to OUTPUT mode

    P3REN |= 0xF0;  // ENABLING the resistors for bits 7 - 4
    P3OUT &= 0x00;  // CLEARING output register. This both clears our outputs on bits 0 - 3, and sets pull-down resistors
                    // for bits 7 - 4
    //---------------- End Configure P3 ----------------

    //---------------- Configure LEDs ----------------
    //Heartbeat LEDs
    P1DIR |= BIT0;            //Config P1.0 (LED1) as output
    P1OUT |= BIT0;            //LED1 = 1 to start

    P6DIR |= BIT6;            //Config P6.6 (LED2) as output
    P6OUT &= ~BIT6;           //LED2 = 0 to start

    //---------------- End Configure LEDs ----------------

    //---------------- Configure Timers ----------------
    //LED Timer
    TB1CTL |= TBCLR;
    TB1CTL |= TBSSEL__ACLK;
    TB1CTL |= MC__UP;
    TB1CCR0 = 32768;
    TB1CCTL0 |= CCIE;
    TB1CCTL0 &= ~CCIFG;

    //Temperature Sample Timer
    TB2CTL |= TBCLR;
    TB2CTL |= TBSSEL__ACLK;
    TB2CTL |= MC__UP;
    TB2CCR0 = 16384;

    TB2CCTL0 |= CCIE;         //enable TB2 CCR0 Overflow IRQ
    TB2CCTL0 &= ~CCIFG;       //clear CCR0 flag
    //---------------- End Timer Configure ---------------

    //---------------- Configure UCB0 I2C ----------------

    // Configure P1.2 (SDA) and P1.3 (SCL) for I2C
    P1SEL0 |= BIT2 | BIT3;
    P1SEL1 &= ~(BIT2 | BIT3);

    // Put eUSCI_B0 into reset mode
    UCB0CTLW0 = UCSWRST;

    // Set as I2C master, synchronous mode, SMCLK source
    UCB0CTLW0 |= UCMODE_3 | UCMST | UCSYNC | UCSSEL_3;

    // Manually adjusting baud rate to 100 kHz  (1MHz / 10 = 100 kHz)
    UCB0BRW = 10;

    // Set slave address
    UCB0I2CSA = LED_ADDRESS;

    // Release reset state
    UCB0CTLW0 &= ~UCSWRST;

    // Enable transmit interrupt
    UCB0IE |= UCTXIE0;
    //---------------- End Configure UCB0 I2C ----------------

    //---------------- Configure UCB1 I2C ----------------

    // Configure P4.6 (SDA) and P4.7 (SCL) for I2C
    P4SEL0 |= BIT6 | BIT7;
    P4SEL1 &= ~(BIT6 | BIT7);

    // Put eUSCI_B0 into reset mode
    UCB1CTLW0 = UCSWRST;

    // Set as I2C master, synchronous mode, SMCLK source
    UCB1CTLW0 |= UCMODE_3 | UCMST | UCSYNC | UCSSEL_3;

    // Manually adjusting baud rate to 100 kHz  (1MHz / 10 = 100 kHz)
    UCB1BRW = 10;

    // Set slave address
    UCB1I2CSA = LCD_ADDRESS;

    // Release reset state
    UCB1CTLW0 &= ~UCSWRST;

    // Enable transmit interrupt
    UCB1IE |= UCTXIE1;
    //---------------- End Configure UCB0 I2C ----------------

    send_I2C_data();

    send_led_i2c();

    __enable_interrupt();       // Enable Global Interrupts
    PM5CTL0 &= ~LOCKLPM5;       // Clear lock bit

    while(1) {}

    return 0;
}

//-------------------------------------------------------------------------------
// Interrupt Service Routines
//-------------------------------------------------------------------------------

//---------------- START ISR_TB0_SwitchColumn ----------------
//-- TB0 CCR0 interrupt, read row data from column, shift roll read column right
#pragma vector = TIMER0_B0_VECTOR
__interrupt void ISR_TB0_SwitchColumn(void)
{

    if(state == 1){ // If in unlocking state
        if(mili_seconds_surpassed >= 5000){
            state = 0; // Set to lock state
            index = 0; // Reset position on input_code
            mili_seconds_surpassed = 0; // Reset timeout counter
            tx_buffer[0] = 8;
            tx_buffer[2] = 0;
            send_I2C_data();
        }else{
            mili_seconds_surpassed++;
        }
    }

    switch (column) {
        case 0:
            P3OUT = 0b00001000; //Enable reading far left column
            break;
        case 1:
            P3OUT = 0b00000100; // Enable reading center left column
            break;
        case 2:
            P3OUT = 0b00000010; // Enable reading center right column
            break;
        default: // Case 3
            P3OUT = 0b00000001; // Enable reading far right column
    }

    if(P3IN > 15){  // If a button is being pressed

        if(state == 0){ // If we're in the locked state, go to unlocking state.
            state = 1;
        }

        if(P3IN & BIT4){    // If bit 4 is receiving input, we're at row 3, so on and so forth
            row = 3;
        }else if(P3IN & BIT5){
            row = 2;
        }else if(P3IN & BIT6){
            row = 1;
        }else if(P3IN & BIT7){
            row = 0;
        }

        key_pressed = keyPad[row][column];
        tx_buffer[2] = key_pressed;

        switch(state){

            case 1: // If unlocking, we populate our input code with each pressed key
                input_code[index] = key_pressed; // Set the input code at index to what is pressed.

                if(index >= 3){ // If we've entered all four digits of input code:
                    index = 0;
                    state = 2; // Initially set state to free
                    mili_seconds_surpassed = 0; // Stop lockout counter
                    int i;
                    for(i = 0; i < 4; i++){ // Iterate through the pass_code and input_code
                        if(input_code[i] != pass_code[i]){ // If an element in pass_code and input_code doesn't match
                            state = 0;                   // Set state back to locked.
                            tx_buffer[0] = 8;
                            tx_buffer[2] = 0;
                            break;
                        }
                    }
                }else{
                    index++; // Shift to next index of input code
                }

                break;

            default:     // If unlocked, we check the individual key press.
                switch(key_pressed){
                    case('D'):
                        state = 0; // Enter locked mode
                        tx_buffer[0] = 0;
                        tx_buffer[1] = 0;
                        transition = 32768;
                        led_index = 0;
                        break;
                    case('A'):
                        state = 3;
                        tx_buffer[0] = 2;
                        break;
                    case('B'):
                        state = 4;
                        tx_buffer[0] = 1;
                        break;
                    case('0'):      // Pattern 0
                        if (state == 4)
                        {
                            tx_buffer[0] = 3;
                            tx_buffer[1] = 0;
                            led_index = 1;
                            state = 2;
                        }
                        else if (state == 3)
                        {
                            window_size = 3;
                            state = 2;
                            tx_buffer[4] = window_size;
                        }
                        break;
                    case('1'):      // Pattern 1
                        if (state == 4)
                        {
                            tx_buffer[0] = 3;
                            tx_buffer[1] = 1;
                            led_index = 2;
                            state = 2;
                        }
                        else if (state == 3)
                        {
                            window_size = 1;
                            state = 2;
                            tx_buffer[4] = window_size;
                        }
                        break;
                    case('2'):      // Pattern 2
                        if (state == 4)
                        {
                            tx_buffer[0] = 3;
                            tx_buffer[1] = 2;
                            led_index = 3;
                            state = 2;
                        }
                        else if (state == 3)
                        {
                            window_size = 2;
                            state = 2;
                            tx_buffer[4] = window_size;
                                                }
                        break;
                    case('3'):      // Pattern 3
                        if (state == 4)
                        {
                            tx_buffer[0] = 3;
                            tx_buffer[1] = 3;
                            led_index = 4;
                            state = 2;
                        }
                        else if (state == 3)
                        {
                            window_size = 3;
                            state = 2;
                            tx_buffer[4] = window_size;
                        }
                        break;
                    case('4'):      // Pattern 4
                        if (state == 4){
                            tx_buffer[0] = 3;
                            tx_buffer[1] = 4;
                            led_index = 5;
                            state = 2;
                        }
                        else if (state == 3)
                        {
                            window_size = 4;
                            state = 2;
                            tx_buffer[4] = window_size;
                        }
                        break;
                    case('5'):      // Pattern 5
                        if (state == 4)
                        {
                            tx_buffer[0] = 3;
                            tx_buffer[1] = 5;
                            led_index = 6;
                            state = 2;
                        }
                        else if (state == 3)
                        {
                            window_size = 5;
                            state = 2;
                            tx_buffer[4] = window_size;
                        }
                        break;
                    case('6'):      // Pattern 6
                        if (state == 4)
                        {
                            tx_buffer[0] = 3;
                            tx_buffer[1] = 6;
                            led_index = 7;
                            state = 2;
                        }
                        else if (state == 3)
                        {
                            window_size = 6;
                            state = 2;
                            tx_buffer[4] = window_size;
                        }
                        break;
                    case('7'):      // Pattern 7
                        if (state == 4)
                        {
                            tx_buffer[0] = 3;
                            tx_buffer[1] = 4;
                            led_index = 8;
                            state = 2;
                        }
                        else if (state == 3)
                        {
                            window_size = 7;
                            state = 2;
                            tx_buffer[4] = window_size;
                        }
                        break;
                    case('8'):
                        if (state == 3)
                        {
                            window_size = 8;
                            state = 2;
                            tx_buffer[0] = 3;
                            tx_buffer[4] = window_size;
                        }
                        break;
                    case('9'):
                        if (state == 3)
                        {
                            window_size = 9;
                            state = 2;
                            tx_buffer[0] = 3;
                            tx_buffer[4] = window_size;
                        }
                    default:
                        break;
                }
                break;
        }

        while(P3IN > 15){} // Wait until button is released
        send_I2C_data();
    send_led_i2c();
    }

    if(P3IN < 16){ // Checks if pins 7 - 4 are on, that means a button is being held down; don't shift columns
        if (++column >= 4) {column = 0;} // Add one to column, if it's 4 reset back to 0.
    }
    TB0CCTL0 &= ~TBIFG;
}
//---------------- End ISR_TB0_SwitchColumn ----------------

//---------------- START ISR_TB1_Heartbeat ----------------
// Heartbeat function
#pragma vector = TIMER1_B0_VECTOR
__interrupt void ISR_TB1_Heartbeat(void)
{
    P1OUT ^= BIT0;               //Toggle P1.0(LED1)
    P6OUT ^= BIT6;               //Toggle P6.6(LED2)
    TB1CCTL0 &= ~CCIFG;          //clear CCR0 flag
}
//---------------- END ISR_TB1_Heartbeat ----------------

//---------------- START ISR_TB2_CCR0 ----------------
// Sample LM19 Temperature
#pragma vector = TIMER2_B0_VECTOR
__interrupt void ISR_TB2_CCR0(void)
{
    if (state == 2)
    {
        start_ADC_conversion();
    }

}

#pragma vector = USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void) {
    if (UCB0IV == 0x18) { // TXIFG0 triggered
            if (tx_index < TX_BYTES) {
                UCB0TXBUF = tx_buffer[tx_index++]; // Load next byte
            } else {
                UCB0CTLW0 |= UCTXSTP; // Send stop condition
                UCB0IE &= ~UCTXIE0;   // Disable TX interrupt after completion
                tx_index = 0;
            }
        }
}

#pragma vector = USCI_B1_VECTOR
__interrupt void USCI_B1_ISR(void) {
    if (UCB1IV == 0x18) { // TXIFG0 triggered

                UCB1TXBUF = led_buffer[led_index];
                UCB1CTLW0 |= UCTXSTP; // Send stop condition
                UCB1IE &= ~UCTXIE1;   // Disable TX interrupt after completion

       }
}

#pragma vector = ADC_VECTOR
__interrupt void ADC_ISR(void) {
    // Store the ADC result in the array
    adc_results[sample_index] = ADCMEM0;
    sample_index++;

    // If we have collected window_size, calculate the average and reset the counter
    if (sample_index >= window_size)
    {

        samples_collected = 1;
        sample_index = 0;

    }

    if (samples_collected == 1)
    {

        get_temperature();

    }
}