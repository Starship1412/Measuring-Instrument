/* 	Digital and Embedded Systems 
	Generates a square wave on P3.6 with adjustable frequency.
	Switches on P2.2 to P2.0 set the frequency, by setting the 
	reload value for timer 2 according to values in a look-up table.
	Switch on P2.7 enables interrupts, so enabling this output.
	In parallel, generates a slow square wave on P3.4 to flash an
	LED - this uses a software delay, independent of interrupts.

	Main program configures timer and then loops, checking switches,
	setting reload value, enabling interrupts and flashing LED.
	Timer 2 ISR changes state of output on P3.6 and clears flag.
	
	Frequency calculation, based on clock frequency 11.0592 MHz:
	Example: For 100 Hz output, want interrupts at 200 Hz, or  
	every 55296 clock cycles.  So set timer 2 reload value to 
	(65536 - 55296) = 10240. 		*/
	 
#include <ADUC841.H>

// define some useful variable types
typedef unsigned char uint8;		// 8-bit unsigned integer
typedef unsigned short int uint16;	// 16-bit unsigned integer

//sbit P2_7 = 0xA7;		// define a port pin - used in an option below 
sfr16 RCAP2 = 0xCA;		// Timer 2 reload register, 16-bit

#define LED			T0		// flashing LED (P3.4 called T0 in header file)
#define BUZZ_PIN	WR		// output on Port 3 pin 6 (WR in header file)
#define LED_BANK	P0		// LEDs are connected to Port 0
#define SWITCHES	P2		// switches are connected to Port 2
#define ENABLE_BIT	0x80	// mask to select bit 7 (enable interrupts)
#define FREQ_MASK 	0x07	// mask to select lowest 3 bits (frequency set)
#define LOW_BYTE	0x00FF	// mask to select low byte of 16-bit word


/*------------------------------------------------
Interrupt service routine for timer 2 interrupt.
Called by the hardware when the interrupt occurs.
------------------------------------------------*/
void timer2 (void) interrupt 5   // interrupt vector at 002BH
{
	BUZZ_PIN = ~BUZZ_PIN;		// invert the output pin
	TF2 = 0;					// clear interrupt flag
}	// end timer2 interrupt service routine


/*------------------------------------------------
Software delay function - argument is number of loops.
------------------------------------------------*/
void delay (uint16 delayVal)
{
	uint16 i;                 // counting variable 
	for (i = 0; i < delayVal; i++)    // repeat  
    {
		  // do nothing
    }
}	// end delay


/*------------------------------------------------
The main C function.  Program execution starts
here after stack initialization.
------------------------------------------------*/
void main (void) 
{
	uint8 swit;     // switch value - 8 bits
	uint8 freqNum;	// frequency number
	uint16 reloadVal;	
	// reload values for 200, 300, 400, 600, 800, 1600, 2400, 3600 Hz
	code uint16 lookUp[8] = { 37888, 47104, 51712, 56320, 58624, 
											62080, 63232, 64000 };
/* 	Note: Standard C would use "const uint16 lookUp[8] ---"
	The "const" qualifier specifies that the array is constant, 
	and the compiler should not allow the program to change it.  
	This compiler would store the array in data memory (like a 
	normal array), and create code to copy the initial values 
	from program memory to data memory at startup.
	The "code" qualifier is an extension to C, similar to const, 
	but telling the compiler to store the array in program memory. */

	// Set switch port for use as input
	SWITCHES = 0xFF;	// output 1 to allow pin to be used as input
/*  Note: The port registers contain 0xFF after reset, but the 
	startup code changes Port 2 to 0x00 in "compact" mode. */

	// Set up timer 2 in timer mode, auto reload, no external control
    T2CON = 0x04;  // all zero except run control
    ET2   = 1;     // enable timer 2 interrupt

	// After setting up, main goes into an infinite loop
	while (1)         // loop forever, repeating tasks
	{
		swit = SWITCHES;		// read switch values
		LED_BANK = swit;		// output on LEDs 

		// Use enable switch to control all interrupts
		EA = ((swit & ENABLE_BIT) == ENABLE_BIT);
		// EA = P2_7;  // simpler alternative?

		// Use 3 switches to set reload value (hence frequency)
		freqNum = swit & FREQ_MASK;		// frequency number
		reloadVal = lookUp[freqNum];	// get reload value from table
		RCAP2 = reloadVal;		// put value in reload register (16-bit)
		// Alternative, using the 8-bit special function register definitions
		//RCAP2L = reloadVal & LOW_BYTE;	// put low byte in reload register
		//RCAP2H = reloadVal >> 8;			// put high byte in reload register
		
		// Change state of LED
		LED = ~LED;
		
		// Delay
		delay (40000);
		
	} // end while(1)
		
}	// end main
