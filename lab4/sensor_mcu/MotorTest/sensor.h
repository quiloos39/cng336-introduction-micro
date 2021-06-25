#ifndef SENSOR_H_
#define SENSOR_H_
#include <avr/sleep.h>
void Sensors_init() {
	//**** Assume necessary DDR are set as input ****//
	
	// prescaling (for sampling) = 2, single conversion mode (defaults)
	ADCSRA |= (1<<ADEN) | (1<<ADIE); // enable ADC and interrupts
	
	// AREF as voltage reference,
	// ADC0 as default input
	// Left justified
	ADMUX |= (1 << ADLAR);
	 	
}

void Sensors_set_input_pin(uint8_t inp) {
	if (inp > 3) inp = 3; // only 4 sensors
	ADMUX &= (0b11100000+inp);
}

// LAST KNOWN SENSOR DATA
volatile uint8_t Sensors_last_logged_data[4] = {0,0,0,0};
volatile uint8_t Sensors_last_logged_idx = 3;

void Sensors_logging_routine() {
	// Assume running in while loop
	
	ADCSRA |= (1<<ADSC); // start conversion
	set_sleep_mode(1); // ADC Noise Reduction sleep mode
	sleep_mode();
}

// Conversion complete interrupt routine
ISR(ADC_vect) {
	if (Sensors_last_logged_idx == 3) {
		Sensors_last_logged_idx = 0;
		} else {
		Sensors_last_logged_idx++;
	}
	// conversion complete, simply load the data
	// and set the next input pin
	Sensors_last_logged_data[Sensors_last_logged_idx] = ADCH;
	Sensors_set_input_pin(Sensors_last_logged_idx);
}



#endif /* INCFILE1_H_ */