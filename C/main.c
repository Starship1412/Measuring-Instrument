#include <ADUC841.h>	// special function register definitions
#include "typedef.h"	// variable type definitions
#define LED_BANK	P0	// bank of LEDs on port 0
#define LED T0  //flashing LED (P3.4 called T0 in header file)
#define SWITCH P2

uint16 voltage_avr; // Global variable for passing data from the interrupt function to the main function
uint16 frequency;
int voltage_difference;

sbit Load = 0xB2; // Set the display signal transmission position

void display (uint8 a, uint8 b);
void displayInitialize (void);
void displaynumber (uint16 number);
void delay (uint16 delayVal);

void main (void) {
	static uint8 previousSWITCH = 0xFF;
	uint8 once = 0;
    uint16 voltage_mv;
	uint16 voltage_slowmax;
	uint16 voltage_slowmin;
	uint16 voltage_difference_show;
	uint16 frequency_show;
	LED_BANK = 0x55;
	displayInitialize();
	LED_BANK = 0xF0;
	SWITCH = 0xFF;
	EA = 1;                       /* Global Interrupt Enable */
	EADC = 1;                     /* ADC Interrupt Enable */
    T2CON = 0x04;                 /* 00000100   TR2 = 1   which means making Timer2 run */
	ADCCON1 = 0x8E;               /* 10001110 */ 
	ADCCON2 = 0x00;
	while(1) {
		switch(SWITCH) {
			case 0xFE: { // Measure DC voltage, in the range 0 V to 2.5 V
                RCAP2H = 0xEA; // Set the starting position of Timer2 according to the frequency of interrupt
                RCAP2L = 0x66;
				if (SWITCH != previousSWITCH)
					previousSWITCH = SWITCH;
				EADC = 0; // When passing data from the interrupt function to the main function using global variables, the ADC interrupt needs to be temporarily disabled.
				voltage_mv = voltage_avr*2500UL/4096; // Convert ADC data to voltage value. UL must be brought, otherwise it will lead to incorrect calculation results because voltage_avr is only 16 bits, and the temporary product result will be more than 16 bits.
				EADC = 1;
				LED_BANK = ~(voltage_mv/10);
				displaynumber(voltage_mv);
				LED = ~LED;  // Change the LED state
				break;
			}
			case 0xFC: { // Measure amplitude of a changeable DC input signal, up to 2.5 V(additional measurement function)
				RCAP2H = 0xEA;
                RCAP2L = 0x66;
				if (SWITCH != previousSWITCH) { // The action performed each time the switch is toggled. This is to allow the program of this case to initialize the DC starting voltage without error each time it is re-executed.
					once = 0;
					previousSWITCH = SWITCH;
				}
				LED_BANK = 0x03;
				EADC = 0;
				voltage_mv = voltage_avr*2500UL/4096;
				EADC = 1;
				if (once == 0 && voltage_mv != 0) { // Initialization of the DC starting voltage
    				voltage_slowmax = voltage_mv;
    				voltage_slowmin = voltage_mv;
    				once++;
				}
				if (voltage_mv != 0) { // Find the peak of DC voltage
					if (voltage_mv > voltage_slowmax)
						voltage_slowmax = voltage_mv;
					if (voltage_mv < voltage_slowmin)
						voltage_slowmin = voltage_mv;
					voltage_difference_show = voltage_slowmax - voltage_slowmin;
				}
				displaynumber(voltage_difference_show);
				break;
			}
			case 0xFD: { // Measure amplitude of a periodic input signal, up to 5 Vp-p
				RCAP2H = 0xFF; // Set the start position of Timer 2 based on an interrupt sampling frequency that is as high as possible
                RCAP2L = 0x48;
				if (SWITCH != previousSWITCH)
					previousSWITCH = SWITCH;
                EADC = 0;
				voltage_difference_show = voltage_difference*2500UL/4096;
				EADC = 1;
				displaynumber(voltage_difference_show);
				break;
			}
            case 0xFB: { // Measure frequency of a periodic input signal, with minimum range 200 Hz to 10 kHz
				RCAP2H = 0xFF;
                RCAP2L = 0x48;
				if (SWITCH != previousSWITCH)
					previousSWITCH = SWITCH;
				LED_BANK = 0xFB;
                EADC = 0;
				frequency_show = frequency;
				EADC = 1;
				displaynumber(frequency_show);
				break;
			}
			default: {
				LED_BANK = 0x0f;
				break;
			}
		}
	delay(20000);		
	}
}

void getADC() interrupt 6 { // 6 * 8 + 3 = 51 = 33H, which means using ADCI interrupt.
	static uint16 ADC_count = 0;
	static uint32 voltage_sum = 0;
	static uint16 maximal_count = 0;
	static uint8 previousSWITCH = 0xFF;
	static int voltage_fastmax = 0;
   	static int voltage_fastmin = 0;
    static int voltage_1 = 0;
    static int voltage_2 = 0;
    static int voltage_3 = 0;
	int voltage;
	uint16 voltage_H;
	uint16 voltage_L;
  	voltage_H = ADCDATAH & 0x0F; // Take 8 high digitals of ADC data
  	voltage_L = ADCDATAL; // Take 8 low digitals of ADC data
	voltage = voltage_H << 8; // Shift 8 digitals from ADCDATAH to 8 high digitals of the voltage
	voltage |= voltage_L; // Assign 8 digitals from ADCDATAL to 8 low digitals of the voltage
  	ADC_count++; // Record the number of interrupt executions
  	if (SWITCH != previousSWITCH) {
      	ADC_count = 0;
		voltage_sum = 0; // To prevent the wrong voltage from being displayed when the switch is switched back.
      	maximal_count = 0; // To prevent the wrong frequency from being displayed at the first half second when the switch is switched back.
      	previousSWITCH = SWITCH;
  	}
  	switch(SWITCH) {
    	case 0xFE: { // Measure DC voltage, in the range 0 V to 2.5 V
        	voltage_sum += voltage;
          	if (ADC_count >= 500) { // The average of the sampled voltages is calculated every half second.
          		voltage_avr = voltage_sum/ADC_count; // Take the average of a large amount of voltage data sampled over a period of time as the display value. This means that the display frequency is greatly reduced, but at the same time ensures that the displayed voltage is consistent and smooth.
          		ADC_count = 0;
          		voltage_sum = 0;
          	}
    		break;
    	}
    	case 0xFC: { // Measure amplitude of a changeable DC input signal, up to 2.5 V
          	voltage_sum += voltage;
          	if (ADC_count >= 500) {
          		voltage_avr = voltage_sum/ADC_count;
          		ADC_count = 0;
          		voltage_sum = 0;
          	}
          	break;
    	}
    	case 0xFD: { // Measure amplitude of a periodic input signal, up to 5 Vp-p
          	if (voltage > voltage_fastmax)
    			voltage_fastmax = voltage;
    		if (voltage < voltage_fastmin)
    			voltage_fastmin = voltage;
          	if (ADC_count >= 15000) { // To ensure that the sampled voltages are as close as possible to the maximum and minimum values, the sampling frequency needs to be as high as possible, but not exceeding the capacity limits of the embedded system. The peak of periodic input signal change is redetermined every half second.
				voltage_difference = voltage_fastmax - voltage_fastmin;
                voltage_fastmax = voltage; // Initialize the peak of periodic input signal through starting from the current voltage value to find the maximum and minimum voltage values before the next half second. This is to prevent measuring the wrong voltage amplitude when the voltage amplitude changes.
                voltage_fastmin = voltage;
          		ADC_count = 0;
          	}
          	break;
    	}
    	case 0xFB: { // Measure frequency of a periodic input signal, with minimum range 200 Hz to 10 kHz
			voltage_3 = voltage_2;
			voltage_2 = voltage_1;
			voltage_1 = voltage;
			if (voltage_3 < voltage_2 && voltage_2 > voltage_1) // Determine the frequency of the periodic input signal by finding the number of occurrences of the extremes over a period of time
				maximal_count++;
			if (ADC_count >= 15000) { // To ensure that the frequency range that can be measured is as large as possible, the sampling frequency needs to be as high as possible, but not exceeding the capacity limits of the embedded system. The frequency of periodic input signal change is redetermined every half second.
				frequency = maximal_count * 2; // Since only the very large values are counted in half a second and not the very small values, it is necessary to multiply by 2 here for the result to be the frequency.
				ADC_count = 0;
				maximal_count = 0; // Initialize the number of occurrences of maximal value for the next new comparison before the next half second
				LED = ~LED;
			}
			break;
    	}
	}
}

void display (uint8 address, uint8 datas) {
	Load = 0; // load goes low at start
	SPIDAT = address; // send address byte
	delay(5); // code to wait until ISPI is 1
	ISPI = 0; // reset ISPI
	SPIDAT = datas; // send data byte
	delay(5); // code to wait until ISPI is 1
	ISPI = 0; // reset ISPI
	Load = 1; // load goes high at end
}
void displayInitialize (void) {
	SPICON = 0x30; // Configure the SPI interface. Choose SPE and SPIM on table 19 SPICON SFR Bit Designations in ADUC841_842_843_revB.
	Load = 1;
	display(0X0F,0x00); // Set Register Display Test in normal operation
	display(0x0C,0x01); // Set Register Shutdown to turn on the display
	display(0x0A,0xFF); // Set Register Intensity max
	display(0x09,0xFF); // Set Register Decode Mode all in 1
	display(0xFF,0x00); // Set Register Display Test off
	display(0x0B,0x03); // Set Register scan limit 4
}
void displaynumber (uint16 number) {
    uint8 digital_address;
	uint8 number_;
	uint8 display_count;
	digital_address = 1;
	for (display_count = 0; display_count < 5; display_count++) {
		number_ = number%10;
		if (digital_address == 4) {
			number_ |= 0x80; // 10000000
			display(digital_address,number_);
		}
		else if (digital_address != 4) {
			display(digital_address,number_);
		}
		number /= 10;
		digital_address++;
	}
}
/*------------------------------------------------
Software delay function - argument is number of loops.
------------------------------------------------*/
void delay (uint16 delayVal) {
	uint16 i;                 // counting variable
	for (i = 0; i < delayVal; i++) {    // repeat
		  // do nothing
    }
}	// end delay