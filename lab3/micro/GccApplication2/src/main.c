#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#include "./timer.h"
#include "./user_interface.h"
#include "./sensor_interface.h"

#define WDT_MS30 0
#define WDT_MS250 1
#define WDT_MS500 2

// Enables Watchdog.
void WDTEnable(uint8_t timer_flag) {
	// Start RESET sequence every 30ms.
	if (timer_flag == WDT_MS30) {
		WDTCR |= (1 << WDCE) | (1 << WDE) | (1 << WDP0);
	} 
	// Start RESET sequence every 250ms.
	else if (timer_flag == WDT_MS250) {
		WDTCR |= (1 << WDCE) | (1 << WDE) | (1 << WDP2);
	}
	// Start RESET sequence every 500ms.
	else if (timer_flag == WDT_MS500) {
		WDTCR |= (1 << WDCE) | (1 << WDE) | (1 << WDP2) | (1 << WDP0);
	}
}

// Sensor reset timer.
ISR (TIMER1_COMPA_vect)
{
	transmitPacket(0 | crc3(0)); // send reset package
}

void enableXMEM() {
	MCUCR |= (1 << SRE); // ExMEM enable.
	XMCRB |= (1 << XMM1) | (1 << XMM0); // Enable pins C[2:0] for external memory use.
}

int main(void) {
	enableXMEM(); // Enable external memory.
	// cli();
	// WDTEnable(2);
	sei(); // Enable global interrupt.
	configureUserInterface(); // Initialize user interface.
	configureSensorInterface(); // Initialize sensor interface.
	set_sleep_mode(SLEEP_MODE_IDLE); //  Set sleep mode to IDLE
	while (1) {
		sleep_mode(); // Put MCU on sleep mode.
	};
	return 0;
}

