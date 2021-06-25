#ifndef TIMER_H_LAB
#define TIMER_H_LAB

#include <stdint.h>
#include <avr/io.h>

#define CLKS_1_0 7812
#define CLKS_5_0 39063

inline void setupTimer1() {
	TCNT1H = 0;
	TCNT1L = 0;
	TCCR1B |= 
		(1 << WGM12) | 
		(1 << CS12)  |
		(1 << CS10);  
	// OCR1A = 39063;
	OCR1A = 1;
	TIMSK |= (1 << OCIE1A);
}

inline void resetTimer1() {
	TCNT1H = 0;	
	TCNT1L = 0;
}



#endif