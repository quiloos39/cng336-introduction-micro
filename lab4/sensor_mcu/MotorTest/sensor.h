#ifndef SENSOR_H_
#define SENSOR_H_
#include <avr/sleep.h>

#define TEMP_IDX 0
#define MOIST_IDX 1
#define WATER_IDX 2
#define BATTERY_IDX 3


uint8_t lcd_busy;

void Sensors_set_input_pin(uint8_t inp) {
	if (inp > 3) 
		inp = 3; // only 4 sensors
	ADMUX &= ~(0b11111);
	ADMUX |= inp;
}

void Sensors_init() {
	//**** Assume necessary DDR are set as input ****//
	
	// prescaling (for sampling) = 2, single conversion mode (defaults)
	// enable ADC and interrupts
	ADCSRA |= 
		(1 << ADEN)  | // A to D convert enable
		(1 << ADIE)  | //  A to D interrupt enable
		
		// Setting prescalar to 64
		(1 << ADPS2) | 
		(1 << ADPS1);  
	
	// AREF as voltage reference,
	// ADC0 as default input
	// Left justified
	ADMUX |= (1 << ADLAR);
	
	lcd_busy = 0;
	Sensors_set_input_pin(0);
}

// LAST KNOWN SENSOR DATA
volatile uint8_t Sensors_last_logged_data[4] = {0xff,0xff,0xff,0xff};
volatile uint8_t Sensors_last_logged_idx = 0;

void Sensors_logging_routine() {
	// Assume running in while loop
	if (!lcd_busy) 
		ADCSRA |= (1 << ADSC); // start conversion}
}

// utility function to convert to data packet
uint8_t convertDataPacket(uint8_t data_idx) {
	// raw_data: 8-bits of raw data from sensors
	// Reading data from Sensors_last_logged_data
	// data_idx represents the type of data
	
	// Temperature
	uint8_t packet;
	
	if (data_idx == TEMP_IDX) {
		packet = Sensors_last_logged_data[TEMP_IDX];
		packet = TEMP_PACKAGE | packet >> 3;
		
	}
	// Moisture
	else if (data_idx == MOIST_IDX) {
		packet = Sensors_last_logged_data[MOIST_IDX];
		packet = MOIST_PACKAGE | packet >> 3;
		
	}
	// Water level
	else if (data_idx == WATER_IDX)  {
		packet = Sensors_last_logged_data[WATER_IDX];
		packet = WATER_PACKAGE | packet >> 3;
		
	}
	// Batter
	else if (data_idx == BATTERY_IDX) {
		packet = Sensors_last_logged_data[BATTERY_IDX];
		packet = BATTERY_PACKAGE | packet >> 3;
	}
	
	return packet;	
}

// Conversion complete interrupt routine
ISR(ADC_vect) {
	// conversion complete, simply load the data
	// and set the next input pin
	uint8_t tmp = ADCL;
	Sensors_last_logged_data[Sensors_last_logged_idx] = ADCH;
	
	if (Sensors_last_logged_idx == 3) {
		Sensors_last_logged_idx = 0;
	} else {
		Sensors_last_logged_idx++;
		
	}


	// disable conversion while printing
	
	
	
	Sensors_set_input_pin(Sensors_last_logged_idx);
}



#endif