/*
 * MotorTest.c
 *
 * Created: 22/06/2021 17:51:32
 * Author : Oscar
 */ 

#define F_CPU 8000000UL

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "motor.h"
#include "sensor.h"
#include "lcd.h"
#include "terminal.h"

Buffer *output_buffer;

void enableSensorTransmit() {
	UCSR0B |= (1 << TXCIE0) | (1 << TXEN0) ;
	UCSR0B &= ~((1 << RXCIE0) | (1 << RXEN0));
}

void enableSensorReceive() {
	UCSR0B |= (1 << RXCIE0) | (1 << RXEN0);
	UCSR0B &= ~((1 << TXCIE0) | (1 << TXEN0));
}

void sensorOutputRead() {
	UDR0 = bufferCursor(output_buffer);
}

void sensorOutputFinished() {
	clearOutputBuffer(output_buffer);
}

ISR(USART0_TX_vect) {
	outputHandler(output_buffer, sensorOutputRead, sensorOutputFinished);
}

ISR(USART0_RX_vect) {
	
}



int main(void)
{
	DDRA = 0; // enable AD[3:0]
	
	
	
	// LCD
	//DDRD = 0xFF;
	DDRE = 0x00000010;
	//LCD l 1= LCD_init(20, 4, &PORTD, &PORTE);
	
	

	UBRR0L = 51;
	enableSensorTransmit();
	output_buffer = newBuffer(128);

	
	
	Sensors_init();
	//LCD_print(l, "test");

	
	
	sei();
    /* Replace with your application code */
    while (1) 
    {
		Sensors_logging_routine();
	
		// get data from Sensors_last_logged_data
	
		
		
	}
}

