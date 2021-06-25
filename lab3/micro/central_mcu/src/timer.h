#ifndef TIMER_H_LAB
#define TIMER_H_LAB

#include <stdint.h>
#include <avr/io.h>

// assumes 1024 prescaling
#define CLKS_0_5 3906
#define CLKS_1_0 7812
#define CLKS_2_0 15625

inline void setupTimer1(uint16_t clk_num) {
	TCNT1H = 0;
	TCNT1L = 0;
	TCCR1B |= (1 << WGM12)|(1 << CS12)|(1 << CS10);  // CTC mode with 1024 prescaling
	OCR1A = clk_num;
	TIMSK |= (1 << OCIE1A);
}

inline void resetTimer1() {
	TCNT1H = 0;
	TCNT1L = 0;
}



#endif