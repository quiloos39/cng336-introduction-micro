#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#include "./timer.h"
#include "./user_interface.h"
#include "./sensor_interface.h"



#define WDT_MS30 0
#define WDT_MS250 1
#define WDT_MS500 2

void WDTEnable(uint8_t timer_flag) {
	if (timer_flag == WDT_MS30) {
		WDTCR |= (1 << WDCE) | (1 << WDE) | (1 << WDP0);
	} else if (timer_flag == WDT_MS250) {
		WDTCR |= (1 << WDCE) | (1 << WDE) | (1 << WDP2);
	} else if (timer_flag == WDT_MS500) {
		WDTCR |= (1 << WDCE) | (1 << WDE) | (1 << WDP2) | (1 << WDP0);
	}
}

ISR (TIMER1_COMPA_vect)
{
	transmitPacket(0 | crc3(0)); // send reset package
}



void enableXMEM() {
	// enable XMEM
	MCUCR |= (1 << SRE);
	XMCRA = 0;
	XMCRB |= (1<<XMM1)|(1<<XMM0); // C[5:7] are available
}


int main(void) {
	cli();
	// WDTEnable(2);
	sei();
	configureUserInterface();
	
	
	enableXMEM();
	configureSensorInterface();
	resetRequest();
	set_sleep_mode(SLEEP_MODE_IDLE);
	while (1) {

		sleep_mode();

	};
	return 0;
}

