
#define F_CPU 8000000UL


#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#include "cfg.h"
#include "crc.h"
#include "motor.h"
#include "sensor.h"
#include "timerlab.h"
#include "lcd.h"
#include "terminal.h"
#include "transmit.h"


LCD lcd;

void updateSensorLCD() {
	lcd = LCD_clear(lcd);
	char s[32];
	if (Sensors_last_logged_data[3] <= 163) { // assuming V_ref = 5, 3.2V=163
		sprintf(s, "BATTERY LOW");
	} else {
		sprintf(s, "T: %.2X  M: %.2X\nW: %.2X  B: %.2X",
				Sensors_last_logged_data[0], Sensors_last_logged_data[1],
				Sensors_last_logged_data[2], Sensors_last_logged_data[3]);
	}
	
	lcd = LCD_print(lcd, s);
}

ISR(TIMER1_COMPA_vect) {
	static uint8_t data_idx = 0;

	uint8_t data_packet;
	uint8_t cmd_packet;
	uint8_t crc11_code;
	
	timer1_counter++;
	if (timer1_counter == timer1_counter_max) {
		timer1_counter = 0;

		cmd_packet = LOG_REQUEST_PACKAGE;
		data_packet = convertDataPacket(data_idx);
		crc11_code = crc11(data_packet, cmd_packet);
		
		if (last_data_ack_received) {
			last_data_ack_received = false;
			last_data_packet = data_packet;
			last_data_packet_idx = data_idx;
		}

		cmd_packet |= crc11_code;
			
		transmitPacket(data_packet);
		transmitPacket(cmd_packet);
		
		data_idx++;

		if (data_idx > BATTERY_IDX) {
			data_idx = 0; // Water idx
		} 
	}
	
	updateSensorLCD();
}


ISR(TIMER3_COMPA_vect) {
	timer3_counter++;
	if (timer3_counter == timer3_motor_on_counter) {
		Motor_setDutyCycle(Sensors_last_logged_data[1]); // Set motor proportional to moisture
		Motor_start();
	} else if(timer3_counter == timer3_counter_max) {
		Motor_stop();
		timer3_counter = 0;
	}
} 

int main() {

	// setup sensor read Timer
	setupTimer1();
	resetTimer1();
	
	// setup motor Timer
	setupTimer3();
	resetTimer3();

	// USART0 Configuration
	UBRR0L = 51;
	DDRA = 0; // enable AD[3:0]
	
	
	configureBuffers();
	Sensors_init();
	Motor_init();
	Motor_stop();
	//Motor_start();
	
	// set up lcd
	DDRD = 0xFF;
	DDRE |= (1<<PINE3) | (1<<PINE4) | (1<<PINE5);
	lcd = LCD_init(16,2, &PORTE, &PORTD);
	updateSensorLCD();
	
	sei();
	
    while (1) {
		Sensors_logging_routine();
		set_sleep_mode(0);
		sleep_mode();
	}
}

