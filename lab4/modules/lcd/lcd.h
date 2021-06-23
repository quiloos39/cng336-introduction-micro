#ifndef LCD_H_
#define LCD_H_

// change depending on connection
#define LCD_RS_BIT 3
#define LCD_RW_BIT 4
#define LCD_EN_BIT 5

#define LCD_DELAY 200 // in micros
#define LCD_MINI_DELAY 100
#define LCD_RESET_CMD 1


#include <stdint.h>
#include <util/delay.h>

typedef struct {
	uint8_t char_num;
	uint8_t line_num;
	volatile uint8_t* cmd_port;
	volatile uint8_t* data_port;

} LCD;


void LCD_sendcmd(LCD l, uint8_t cmd) {
	*(l.cmd_port) &= ~(1<<LCD_RS_BIT | 1<<LCD_RW_BIT); // RS,RW = 0
	*(l.cmd_port) |= 1<<LCD_EN_BIT;
	*(l.data_port) = cmd;
	_delay_us(LCD_DELAY);
	*(l.cmd_port) &= ~(1<<LCD_EN_BIT);
	_delay_us(LCD_MINI_DELAY);
}
LCD LCD_init(uint8_t char_num, uint8_t line_num, 
			 volatile uint8_t* cmd_port, volatile uint8_t* data_port) {
	LCD l;
	l.char_num = char_num;
	l.line_num = line_num;
	l.cmd_port = cmd_port;
	l.data_port = data_port;
	
	LCD_sendcmd(l, 0x38); // set up commands
	LCD_sendcmd(l, 0x0E);
	
	return l;
}


void LCD_clear(LCD l) {
	LCD_sendcmd(l, 0x1);
}

void LCD_putchar(LCD l, uint8_t ch) {
	*(l.cmd_port) |= 1<<LCD_RS_BIT;
	*(l.cmd_port) &= ~(1<<LCD_RW_BIT); // RW = 0
	*(l.data_port) = ch;
	*(l.cmd_port) |= 1<<LCD_EN_BIT;

	_delay_us(LCD_DELAY);
	*(l.cmd_port) &= ~(1<<LCD_EN_BIT);
	_delay_us(LCD_MINI_DELAY);
}

void LCD_print(LCD l, uint8_t str[]) {
	for (uint8_t i = 0; str[i] != '\0'; i++)
		LCD_putchar(l, str[i]);
}

#endif
