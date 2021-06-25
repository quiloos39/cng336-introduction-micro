#define F_CPU 8e6
#define BUFFER_SIZE 80

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include "./lcd.h"
#include "./keyboard.h"
#include "./timerlab.h"

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

typedef struct {
	char message[BUFFER_SIZE];
	uint8_t read_head;
	uint8_t write_head;
	bool locked;
} Buffer;

volatile Buffer inputBuffer;

Buffer initBuffer() {
	Buffer buffer;
	buffer.read_head = 0;
	buffer.write_head = 0;
	buffer.message[0] = '\0';

	return buffer;

}

void pushBuffer(Buffer *b, char data) {
	b->message[b->write_head++] = data;
	if (b->write_head == BUFFER_SIZE) {
		b->write_head = 0;
	}
}

uint8_t popBuffer(Buffer *b) {
	uint8_t data = b->message[b->read_head++];
	if (b->read_head == BUFFER_SIZE) {
		b->read_head = 0;
	}

	return data;
}


void resetBuffer(Buffer *b) {
	b->read_head = 0;
	b->write_head = 0;
	b->message[0] = '\0';
}	

// Last entry not found.
//      |
//    1) Memory Dump\n 2)\0
// USART0 Receive from Central Data Logger MCU
ISR(USART0_RX_vect) {
	char input = UDR0;
	pushBuffer(&inputBuffer, input);
	if (input == '\0') {
		resetBuffer(&inputBuffer);
	}
}

ISR(TIMER1_COMPA_vect) {
	char data = popBuffer(&inputBuffer);
	lcd = LCD_putchar(lcd, data);
}

ISR(INT7_vect) {
	char userInput = getPressedKey(); // Notice returns ascii char not number.
	if (userInput == '#') {
		// LCD_clear(lcd);
		UDR0 = '\r';
	} else {
		UDR0 = userInput;
	}
}

int main(void)
{
	setupTimer1();
	resetTimer1();
	inputBuffer = initBuffer();
	
	
	configureUSART0();
	configureLCDPorts();
	lcd = LCD_init(20, 4, &PORTB, &PORTA);
	configureKeyboard();
	sei();
	while (true);
}

