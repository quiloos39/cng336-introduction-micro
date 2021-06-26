#define F_CPU 8e6
#define BUFFER_SIZE 128

#include <avr/io.h>
#include <avr/sleep.h>
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
	uint8_t size;

} Buffer;

volatile Buffer inputBuffer;

Buffer initBuffer() {
	Buffer buffer;
	buffer.read_head = 0;
	buffer.write_head = 0;
	buffer.size = 0;
	return buffer;

}

void pushBuffer(Buffer *b, char data) {
	b->message[b->write_head] = data;
	b->write_head++;
	if (b->write_head == BUFFER_SIZE) {
		b->write_head = 0;
	}
	
	b->size++;
}

uint8_t popBuffer(Buffer *b) {
	
	uint8_t data = b->message[b->read_head];
	//b->message[b->read_head] = '\0';
	b->read_head++;
	
	if (b->read_head == BUFFER_SIZE) {
		b->read_head = 0;
	}
	b->size--;
	return data;
}


void resetBuffer(Buffer *b) {
	b->read_head = 0;
	b->write_head = 0;
	b->size = 0;
}	

// Last entry not found.
//      |
//    1) Memory Dump\n 2)\0
// USART0 Receive from Central Data Logger MCU
ISR(USART0_RX_vect) {
	char input = UDR0;
	pushBuffer(&inputBuffer, input);
}

ISR(TIMER1_COMPA_vect) {
	// only purpose is to periodically wake up 
}

ISR(INT7_vect) {
	char userInput = getPressedKey(); // Notice returns ascii char not number.
	if (userInput == '#') {
		lcd = LCD_clear(lcd);
		LCD_sendcmd(lcd, LCD_LINE_1);
		UDR0 = '\r';
	} else {
		UDR0 = userInput;
		lcd = LCD_putchar(lcd, userInput);
	}
}

int main(void)
{
	configureLCDPorts();
	lcd = LCD_init(20, 4, &PORTB, &PORTA);

	setupTimer1();
	resetTimer1();
	inputBuffer = initBuffer();
	
	configureKeyboard();
	configureUSART0();
	sei();
	while (true) {
		sleep_mode();
		
		while (inputBuffer.size != 0)  {
			char data = popBuffer(&inputBuffer);
			lcd = LCD_putchar(lcd, data);
		}
	};
}

