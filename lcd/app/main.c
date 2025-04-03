#include <msp430.h> 

// Port definitions
#define PXOUT P1OUT
#define PXSEL0 P1SEL0
#define PXSEL1 P1SEL1
#define PXDIR P1DIR

// Pin definitions
#define D4 BIT0
#define D5 BIT1
#define D6 BIT4
#define D7 BIT5

#define E BIT6
#define RS BIT7

// I2C definitions
#define ADDRESS 0x01    // Address for microcontroller

// LCD Variables
int pattern_index, state_index = 0;

int temperature_int, temperature_dec, window_size = 0;

typedef enum state_enum {LOCKED, SET_PATTERN, SET_WINDOW, DISPLAY_PATTERN};

typedef enum pattern_enum {STATIC, BREAK, UP_COUNTER, IN_AND_OUT, DOWN_COUNTER, ROTATE_1_LEFT, ROTATE_7_LEFT, FILL_LEFT};

char states_array[3][20] = {"", "Set Pattern", "Set Window Size"};

char patterns_array[9][20] = {
    "static", "break", "up counter", "in and out", "down counter", "rotate 1 left", "rotate 7 left", "fill left", ""
};



void lcd_pulse_enable(){
    // Pulses the enable pin so LCD knows to take next nibble.
    PXOUT |= E;         // Enable Enable pin
    PXOUT &= ~E;        // Disable Enable pin
}

void lcd_send_nibble(char nibble){
    // Sends out four bits to the LCD by setting the last four pins (D4 - 7) to the nibble.

    PXOUT &= ~(D4 | D5 | D6 | D7); // Clear the out bits associated with our data lines.

    // For each bit of the nibble, set the associated data line to it.
    if (nibble & BIT0) PXOUT |= D4;
    if (nibble & BIT1) PXOUT |= D5;
    if (nibble & BIT2) PXOUT |= D6;
    if (nibble & BIT3) PXOUT |= D7;

    //PXOUT = (PXOUT &= 0xF0) | (nibble & 0x0F); // First we clear the first four bits, then we set them to the nibble.
    lcd_pulse_enable();  // Pulse enable so LCD reads our input
}

void lcd_send_command(char command){
    // Takes an 8-bit command and sends out the two nibbles sequentially.
    PXOUT &= ~RS; // CLEAR RS to set to command mode
    lcd_send_nibble(command >> 4); // Send upper nibble by bit shifting it to lower nibble
    lcd_send_nibble(command & 0x0F); // Send lower nibble by clearing upper nibble.
}

void lcd_send_data(char data){
    // Takes an 8-bit data and sends out the two nibbles sequentially.
    PXOUT |= RS; // SET RS to set to data mode
    lcd_send_nibble(data >> 4); // Send upper nibble by bit shifting it to lower nibble
    lcd_send_nibble(data & 0x0F); // Send lower nibble by clearing upper nibble.
}

void lcd_print_sentence(char *str){
    // Takes a string and iterates character by character, sending that character to be written out, until \0 is reached.
    while(*str){
        lcd_send_data(*str);
        str++;
    }
}

void lcd_clear(){
    // Clearing the screen is temperamental and requires a good delay, this is pretty arbitrary with a good safety margin.
    lcd_send_command(0x01);   // Clear display
    __delay_cycles(2000);   // Clear display needs some time, I'm aware __delay_cycles generally isn't advised, but it works in this context
}

void lcdInit(){
    // Initializes the LCD to 4 bit mode, 2 lines 5x8 font, with enabled display and cursor,
    // clear the display, then set cursor to proper location.

    PXOUT &= ~RS; // Explicitly set RS to 0 so we are in command mode

    // We need to send the code 3h, 3 times, to properly wake up the LCD screen
    lcd_send_nibble(0x03);
    lcd_send_nibble(0x03);
    lcd_send_nibble(0x03);

    lcd_send_nibble(0x02);    // Code 2h sets it to 4-bit mode after waking up

    lcd_send_command(0x28);   // Code 28h sets it to 2 line, 5x8 font.

    lcd_send_command(0x0C);   // Turns display on, turns cursor off, turns blink off.

    lcd_send_command(0x06);   // Increments cursor on each input

    lcd_clear();             // Clear display
}

void lcd_write(){
    /*  Ultimately dictates what will be present on screen after an I2C transmission.
        I2C should come in five bytes, state_index, pattern_index, temperature_int, temperature_dec, and window_size.
        state_index -> Integer value corresponding to four states device can be in.
        State 0 = Locked. 1 = Set Pattern. 2 = Set Window. 3 = Display Pattern
        pattern_index -> Integer value corresponding to a pattern in patternArray. Pattern 0 is static, so index 0 is static.
        Pattern 8 is empty, and should be used when there is no pattern being displayed.
        temperature_int -> Represents integer portion of temperature in Celsius.
        temperature_dec -> Represents decimal portion of temperature in Celsius

        In the locked state, the display should display nothing.

        In Set Pattern, the display will display the "Set Pattern" Query

        In Set Window, the display will display the "Set Window Size" Query

        In Display Pattern, the display will display the current pattern, which may be empty if none
        is selected. In this case, use the number 8 for the pattern index, as that corresponds to the empty "".
        This should be treated as the default unlocked state.
    */
    lcd_clear(); // Clear Display

    if (state_index == LOCKED){
        return;
    }

    lcd_send_command(0x80); // Set cursor to line 1 position 1

    if (state_index == DISPLAY_PATTERN){
        lcd_print_sentence(patterns_array[pattern_index]);
    }else{
        lcd_print_sentence(states_array[state_index]);
    }

    char temp_string[7]; // Buffer for converting int temp value to string
    int i = 0;

    temp_string[i++] = (temperature_int / 10) + '0';  // Tens place
    temp_string[i++] = (temperature_int % 10) + '0';  // Ones place
    temp_string[i++] = '.';
    temp_string[i++] = (temperature_dec % 10) + '0';  // First decimal place
    temp_string[i++] = 0b11011111; // Degrees symbol
    temp_string[i++] = 'C';



    lcd_send_command(0xC0); // Set cursor to line 2 position 1
    lcd_print_sentence("T=");
    lcd_print_sentence(temp_string);

    lcd_send_command(0xCF); // Set cursor to line 2 position 16
    lcd_print_sentence("j");

    lcd_send_command(0x80); // Return cursor to line 1 position 1
}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    //---------------- Configure LCD Ports ----------------

    // Configure Port for digital I/O
    PXSEL0 &= 0x00;
    PXSEL1 &= 0x00;

    PXDIR |= 0XFF;  // SET all bits so Port is OUTPUT mode

    PXOUT &= 0x00;  // CLEAR all bits in output register
    //---------------- End Configure Ports ----------------

    //---------------- Configure UCB0 I2C ----------------

    // Configure P1.2 (SDA) and P1.3 (SCL) for I2C
    P1SEL0 |= BIT2 | BIT3;
    P1SEL1 &= ~(BIT2 | BIT3);

    UCB0CTLW0 = UCSWRST;                 // Put eUSCI in reset
    UCB0CTLW0 |= UCMODE_3 | UCSYNC;      // I2C mode, synchronous mode
    UCB0I2COA0 = ADDRESS | UCOAEN;       // Set slave address and enable
    UCB0CTLW0 &= ~UCSWRST;               // Release eUSCI from reset
    UCB0IE |= UCRXIE0;                   // Enable receive interrupt
    //---------------- End Configure UCB0 I2C ----------------

    PM5CTL0 &= ~LOCKLPM5;       // Clear lock bit
    __bis_SR_register(GIE);     // Enable global interrupts

    lcdInit();

    while(1){

    }

    return 0;
}

//-------------------------------------------------------------------------------
// Interrupt Service Routines
//-------------------------------------------------------------------------------

#pragma vector=USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void) {
    //ISR For receiving I2C transmissions
    /* The microcontroller expects four bytes to be transmitted from the master.
     * [state], [pattern], [temperature_int], [temperature_dec], [window_size]
     * These bytes are sequentially added to the pattern_index, period_index, and key values.
     *
     * These values are then processed by lcd_write(), where more information can be found about their handling.
     */
    static int byte_count = 0;
    if(UCB0IV == 0x16){  // RXIFG0 Flag, RX buffer is full and can be processed
        switch(byte_count){
            case 0:
                state_index = UCB0RXBUF;
                break;
            case 1:
                pattern_index = UCB0RXBUF;
                break;
            case 2:
                temperature_int = UCB0RXBUF;
                break;
            case 3:
                temperature_dec = UCB0RXBUF;
                break;
            case 4:
                window_size = UCB0RXBUF;
                break;
            default:
                break;
        }
        byte_count++;
        if(byte_count >= 5){
            byte_count = 0;
            lcd_write();
        }
    }
}
