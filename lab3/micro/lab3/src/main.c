#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "./sensor_interface.h"

int main(void)
{
	sei();
	configureUserInterface();
	// displayMenu();
	configureSensorInterface();
	set_sleep_mode(SLEEP_MODE_IDLE);
	while (1) {
		sleep_mode();
	};
	return 0;
}

