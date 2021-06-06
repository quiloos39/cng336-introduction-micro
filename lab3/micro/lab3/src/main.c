#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "./user_interface.h"
#include "./sensor_interface.h"



int main(void)
{
	sei();
	configureUserInterface();
	// displayMenu();
	configureSensorInterface();
	transmitPacket(0 | crc3(0)); // send reset package
	
	set_sleep_mode(SLEEP_MODE_IDLE);
	while (1) {
		sleep_mode();
	};
	return 0;
}

