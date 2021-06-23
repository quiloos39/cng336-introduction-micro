#ifndef KEYBOARD_H_
#define KEYBOARD_H_
#define CHECK_PIN(PORT, PIN) ((PORT & (1 << PIN)) >> PIN)

#include "util/delay.h"

const char KEYPAD[4][3] = {
	{
		'1', '2', '3'
	},
	{
		'4', '5', '6'
	},
	{
		'7', '8', '9'
	},
	{
		'*', '0', '#'
	}
};

void configureKeyboard() {
	// PORTC[0:6] = Input
	DDRC &= ~(
		(1 << PINC3) | // A
		(1 << PINC4) | // B
		(1 << PINC5) | // C
		(1 << PINC6)   // D
	);
	
	// PORTC[0:3] = Output
	DDRC |= (
		(1 << PINC0) | // 1
		(1 << PINC1) | // 2
		(1 << PINC2)   // 3
	);
	
	// PORTC[0:3] = 0
	PORTC &= ~(
		(1 << PINC0) |
		(1 << PINC1) |
		(1 << PINC2)
	);
	
	// Enable interrupt 7 when keys is pressed
	
	// Enabling Pull-up mode.
	DDRE &= ~((PINE7 << 1));
	PORTE |= ((1 << PINE7));
	
	// Interrupt 7 enable.
	EIMSK |= (1 << INT7);
}


uint8_t findPressedRow() {
	uint8_t data = PINC;
	if (!CHECK_PIN(data, 3)) {
		return 1;
		
	} else if (!CHECK_PIN(data, 4)) {
		return 2;
		
	} else if (!CHECK_PIN(data, 5)) {
		return 3;
		
	} else if (!CHECK_PIN(data, 6)) {
		return 4;
	}
	
	return 0;
}

uint8_t findPressedColumn() {
	uint8_t data;
	uint8_t column;
	
	// Column 1
	// Output: 011
	PORTC &= ~((1 << PINC0));
	PORTC |= ((1 << PINC1));
	PORTC |= (1 << PINC2);
	
	data = PINC;
	
	if (
		!CHECK_PIN(data, 3) ||
		!CHECK_PIN(data, 4) ||
		!CHECK_PIN(data, 5) ||
		!CHECK_PIN(data, 6)
	) {
		column = 1;
	}
	

	_delay_us(1000);
	
	// Column 2
	// Output: 101
	PORTC |= ((1 << PINC0));
	PORTC &= ~((1 << PINC1));
	PORTC |= (1 << PINC2);
	
	
	data = PINC;
	
	if (
		!CHECK_PIN(data, 3) ||
		!CHECK_PIN(data, 4) ||
		!CHECK_PIN(data, 5) ||
		!CHECK_PIN(data, 6)
	) {
		column = 2;
	}
	
	_delay_us(1000);
	
	// Column 3
	// Output: 110
	PORTC |= ((1 << PINC0));
	PORTC |= (1 << PINC1);
	PORTC &= ~((1 << PINC2));
	
	data = PINC;
	
	if (
		!CHECK_PIN(data, 3) ||
		!CHECK_PIN(data, 4) ||
		!CHECK_PIN(data, 5) ||
		!CHECK_PIN(data, 6)
	) {
		column = 3;
	}
	
	_delay_us(1000);
	
	// Reseting back to original output
	// Output: 000
	PORTC &= ~(
		(1 << PINC0) |
		(1 << PINC1) |
		(1 << PINC2)
	);

	return column;
}

char getPressedKey() {
	uint8_t row = findPressedRow();
	uint8_t col = findPressedColumn();
	return KEYPAD[row - 1][col - 1];
}

#endif