#define F_CPU 8e6

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include "./lcd.h"
#include "./keyboard.h"

volatile LCD lcd;

void configureLCDPorts() {
	// Data ports A[0:7], Output
	DDRA = 0xFF;
	
	// Control ports B[3:5], Output
	DDRB = (
		(1 << PINB3) |
		(1 << PINB4) |
		(1 << PINB5)
	);
}

void configureUSART0() {
	UBRR0L = 51;
	UCSR0B |= (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);
}


// USART0 Receive from Central Data Logger MCU
ISR(USART0_RX_vect) {
	char input = UDR0;
	// If input == Start of text (check ascii)
	lcd = LCD_putchar(lcd, input);
}


ISR(INT7_vect) {
	char userInput = getPressedKey(); // Notice returns ascii char not number.
	if (userInput == '#') {
		LCD_clear(lcd);
		UDR0 = '\r';
	} else {
		UDR0 = userInput;
	}
}

int main(void)
{
	configureUSART0();
	configureLCDPorts();
	lcd = LCD_init(20, 4, &PORTB, &PORTA);
	configureKeyboard();
	sei();
	while (true);
}

