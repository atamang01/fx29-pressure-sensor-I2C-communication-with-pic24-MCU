/* 
 * File:   I2C_code.c
 * Author: atamang
 *
 * Created on February 9, 2022, 4:46 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include    "config.h"

void us_delay(int time) // micro seocnd delay
{
    TMR1 = 0;
    T1CON = 0x8010; // pre scale of 8
    while (TMR1 < 2 * time) // 0.000001/(1/16,000,000)*8)=2
    {
    }
}

void ms_delay(int N) // miliseocnd dlay
{
    TMR2 = 0; //clear timer,  use timer 2 sice timer 1 is used by us delay
    T2CON = 0x8030; // pre scale of 256
    while (TMR2 < 62.5 * N) // 0.001/(1/16,000,000)*256)=62.5
    {
    }
}

char ReadLCD(int addr) {
    // As for dummy read, see 13.4.2, the first read has previous value in PMDIN1
    int dummy;
    while (PMMODEbits.BUSY); // wait for PMP to be available
    PMADDR = addr; // select the command address
    dummy = PMDIN1; // initiate a read cycle, dummy
    while (PMMODEbits.BUSY); // wait for PMP to be available
    return ( PMDIN1); // read the status register
} // ReadLCD
// In the following, addr = 0 -> access Control, addr = 1 -> access Data
#define BusyLCD() ReadLCD( 0) & 0x80 // D<7> = Busy Flag
#define AddrLCD() ReadLCD( 0) & 0x7F // Not actually used here
#define getLCD() ReadLCD( 1) // Not actually used here.

void WriteLCD(int addr, char c) {
    while (BusyLCD());
    while (PMMODEbits.BUSY); // wait for PMP to be available
    PMADDR = addr;
    PMDIN1 = c;
} // WriteLCD
// In the following, addr = 0 -> access Control, addr = 1 -> access Data
#define putLCD( d) WriteLCD( 1, (d))
#define CmdLCD( c) WriteLCD( 0, (c))
#define HomeLCD() WriteLCD( 0, 2) // See HD44780 instruction set in
#define ClrLCD() WriteLCD( 0, 1) // Table 9.1 of text book

void InitLCD(void) {
    // PMP is in Master Mode 1, simply by writing to PMDIN1 the PMP takes care
    // of the 3 control signals so as to write to the LCD.
    PMADDR = 0; // PMA0 physically connected to RS, 0 select Control register
    PMDIN1 = 0b00111000; // 8-bit, 2 lines, 5X7. See Table 9.1 of text Function set
    ms_delay(1); // 1ms > 40us
    PMDIN1 = 0b00001100; // ON, cursor off, blink off
    ms_delay(1); // 1ms > 40us
    PMDIN1 = 0b00000001; // clear display
    ms_delay(2); // 2ms > 1.64ms
    PMDIN1 = 0b00000110; // increment cursor, no shift
    ms_delay(2); // 2ms > 1.64ms
} // InitLCD

void InitPMP(void) {
    // PMP initialization. See my notes in Sec 13 PMP of Fam. Ref. Manual
    PMCON = 0x8303; // Following Fig. 13-34. Text says 0x83BF (it works) *
    PMMODE = 0x03FF; // Master Mode 1. 8-bit data, long waits.
    PMAEN = 0x0001; // PMA0 enabled
}

void putsLCD(char *s) {
    while (*s) putLCD(*s++); // See paragraph starting at bottom, pg. 87 text
} //putsLCD

void SetCursorAtLine(int i) {
    int k;
    if (i == 1)
        CmdLCD(0x80); // Set DDRAM (i.e., LCD) address to upper left (0x80 | 0x00)
    else if (i == 2)
        CmdLCD(0xC0); // Set DDRAM (i.e., LCD) address to lower left (0x80 | 0x40)
    else {
        TRISA = 0x00; // Set PORTA<7:0> for output.
        for (k = 1; k < 20; k++) // Flash all 7 LED's @ 5Hz. for 4 seconds.
        {
            PORTA = 0xFF;
            ms_delay(100); // 100 ms for ON then OFF yields 5Hz
            PORTA = 0x00;
            ms_delay(100);
        }
    }
}

/*END OF LCD DISPLAY FUNCTIONS*/


void I2Cinit(int BRG) {
    I2C1BRG = BRG; //See PIC24FJ128GA010 data sheet, Table 16.1 pg. 139
    while (I2C1STATbits.P); // Check buss idle, see 5.1 of I2C document.
    // It works, not sure its needed.
    I2C1CONbits.A10M = 0; // 7-bit address mode (Added 8-14-17)
    I2C1CONbits.I2CEN = 1; // enable module
}

void I2CStart(void) {
    us_delay(10); // delay to be safe
    I2C1CONbits.SEN = 1; // Initiate Start condition see pg. 21 of I2C man. DS70000195F
    while (I2C1CONbits.SEN); // wait for Start condition complete See sec. 5.1
    us_delay(10); // delay to be safe
}

void I2CStop(void) {
    us_delay(10); // delay to be safe
    I2C1CONbits.PEN = 1; // see 5.5 pg. 27 of Microchip I2C manual DS70000195F
    while (I2C1CONbits.PEN); // This is at hardware level, & I suspect fast.
    us_delay(10); // delay to be safe
}

void I2Csendbyte(char data) {
    while (I2C1STATbits.TBF); //wait if buffer is full
    I2C1TRN = data; // pass data to transmission register
    us_delay(10); // delay to be safe
}

void I2Cgetbyte(void) {
    I2C1CONbits.RCEN = 1; // Set RCEN, Enables I2C Receive mode
    while (!I2C1STATbits.RBF); //wait for byte to shift into I2C1RCV register
    us_delay(10);
    // I2C1CONbits.ACKDT = 1; // Master sends Acknowledge
    //us_delay(10); 
    I2C1CONbits.ACKEN = 0;
    us_delay(10); // delay to be safe
    // return (I2C1RCV);
}

void I2CRStart(void) {
    us_delay(10); // delay to be safe
    I2C1CONbits.RSEN = 1; // Initiate Start condition see pg. 21 of I2C man. DS70000195F
    while (I2C1CONbits.RSEN); // wait for Start condition complete See sec. 5.1
    us_delay(10); // delay to be safe
}


//output function 

//output calculation to lb
float output_fun(uint8_t Force_applied) {
    float output_count = 0;
    int output_max=8000; 
    int output_min = 1000; 
    //int Force_applied; 
    int Rated_force_range = 50;
    float temp = 0;
    float temp2 = 0;
    float FORCE;
    int offset = 3; 

    //calculate output(count) equation )
    temp = ((output_max - output_min) / Rated_force_range) * Force_applied;
    output_count = temp + 1000; 

    //calculate the force 
    temp2 = (output_count - output_min) / (output_max - output_min);
    FORCE = temp2*Rated_force_range;
    
    float pound = 0.224; //1 N = 0.224809
    float lb; 
    lb =  (FORCE-offset); 
    
    float a_load = 226.796; //8oz to grams
    float a_out = 3.00; //output
    float b_load = 453.592; //1lb 
    float b_out = 5.00; 
    
    float out; 
    out = ((b_load-a_load)/(b_out-a_out))*(lb);
    

     return out;
}





void main(void) {
    int hibyte, lowbyte; //variable for high byte and low byte
    char MS_digit, LS_digit, Mid_digit; //char variables
    TRISA = 0x0; //portA as output
    float output; 
    int offset = 0; 

    //  int counter = 1;
    I2Cinit(157); // 157
    while (1) {

        I2CStart(); // start the I2C
        I2Csendbyte(0x50); // send write command to the I2C
        us_delay(100); // delay to be safe
        I2Csendbyte(0x00); // send low bye
        us_delay(100); //100 us delay
        I2Csendbyte(0x01);
        ms_delay(400); //(100); 
        //I2CStop();        // stop the I2C

        //  us_delay(100);
        I2CRStart();
        //  us_delay(100); 
        I2Csendbyte(0x51);
        us_delay(100);
        I2Cgetbyte();
        //I2C1CONbits.ACKEN = 1; // Master sends Acknowledge
        hibyte = I2C1RCV; //force

        us_delay(200);
        I2Cgetbyte();
        //  I2C1CONbits.ACKEN = 1; // Master sends Acknowledge
        lowbyte = I2C1RCV; 

        us_delay(200);
        I2CStop();

        PORTA = hibyte; //highbyte sent to porta to display on LED's

        MS_digit = hibyte / 100; // gives 125/100 assuming 125 was temp in C
        Mid_digit = (hibyte / 10) % 10; // to get mid byte divide by 10 gives 12 and %10 gives 2
        LS_digit = hibyte % 10; // gives 125 modulo 10= 5 which is LS digit

        
        int16_t bytes; 
        
        bytes = hibyte<<8|lowbyte; 
        
        //float total; 
       // total =  (hibyte)*(5/1023); 
        char LCDchar[20]; // Create an array for 8 characters
        output = output_fun(hibyte);
        sprintf(LCDchar, " %0.2f ", output); // Convert T_decC to FP, put in LCDchar
        ms_delay(32); // Just like in LCD lab, probably not needed.
        InitPMP(); // Initialize the Parallel Master Port
        InitLCD(); // Initialize the LCD
        SetCursorAtLine(1); // Set to first line.
        putsLCD("WEIGHT: "); // Put message on first line of LCD
        CmdLCD(0x0C); //show on Screen; 

        SetCursorAtLine(2); // Set to start of second lind
        putsLCD(LCDchar); // Floating Point Temperature on second line.
        CmdLCD(0x0C); //show on Screen; 
       // ms_delay(1000);

        

        ms_delay(200); //delay

    }

    // return (EXIT_SUCCESS);
}

