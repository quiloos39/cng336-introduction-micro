#ifndef TIMER_H_LAB
#define TIMER_H_LAB

#include <stdint.h>
#include <avr/io.h>

#define CLKS_1_0 7812
#define CLKS_5_0 39063

volatile uint8_t timer1_counter_max = 2; // for 5*2 seconds
volatile uint8_t timer1_counter = 0;

volatile uint8_t timer3_counter_max = 3; // for 10 seconds
volatile uint8_t timer3_motor_on_counter = 2; // when counter == 2, turn on motor
volatile uint8_t timer3_counter = 0;

// Timer1 = Sensor logging
inline void setupTimer1() {
	TCNT1H = 0;
	TCNT1L = 0;
	TCCR1B |= 
		(1 << WGM12) | 
		(1 << CS12)  |
		(1 << CS10);  
	// OCR1A = 39063;
	//OCR1A = CLKS_1_0;
	OCR1A = CLKS_5_0;
	
	TIMSK |= (1 << OCIE1A);
}

inline void resetTimer1() {
	TCNT1H = 0;	
	TCNT1L = 0;
}

inline void setupTimer3() {
	TCNT3H = 0;
	TCNT3L = 0;
	TCCR3B |=
	(1 << WGM12) |
	(1 << CS12)  |
	(1 << CS10);
	// OCR3A = 39063;
	//OCR3A = CLKS_1_0;
	OCR3A = CLKS_1_0;
	
	ETIMSK |= (1 << OCIE3A);
}

inline void resetTimer3() {
	TCNT3H = 0;
	TCNT3L = 0;
}




#endif