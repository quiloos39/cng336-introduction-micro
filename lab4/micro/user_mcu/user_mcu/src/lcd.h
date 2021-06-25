#ifndef LCD_H_
#define LCD_H_

// change depending on connection
#define LCD_RS_BIT 3
#define LCD_RW_BIT 4
#define LCD_EN_BIT 5

#define LCD_LONG_DELAY 2000
// #define LCD_DELAY 2000 // in micros
#define LCD_DELAY 200 // in micros
#define LCD_MINI_DELAY 200
#define LCD_RESET_CMD 1

#define LCD_LINE_1 0x80
#define LCD_LINE_2 0xC0
#define LCD_LINE_3 0x94
#define LCD_LINE_4 0xD4

#include <stdint.h>
#include <util/delay.h>

typedef struct {
	uint8_t char_num;
	uint8_t line_num;
	volatile uint8_t* cmd_port;
	volatile uint8_t* data_port;
	
	uint8_t cursor_pos;

} LCD;


void LCD_sendcmd(LCD l, uint8_t cmd) {
	*(l.cmd_port) &= ~(1<<LCD_RS_BIT | 1<<LCD_RW_BIT); // RS,RW = 0
	*(l.cmd_port) |= 1<<LCD_EN_BIT;
	*(l.data_port) = cmd;
	_delay_us(LCD_DELAY);
	*(l.cmd_port) &= ~(1<<LCD_EN_BIT);
	_delay_us(LCD_MINI_DELAY);
}

LCD LCD_init(uint8_t char_num, uint8_t line_num, volatile uint8_t* cmd_port, volatile uint8_t* data_port) {
	LCD l;
	l.char_num = char_num;
	l.line_num = line_num;
	l.cmd_port = cmd_port;
	l.data_port = data_port;
	l.cursor_pos = LCD_LINE_1;
	
	LCD_sendcmd(l, 0x38); // set up commands
	LCD_sendcmd(l, 0x0E);
	LCD_sendcmd(l, l.cursor_pos);
	return l;
}


void LCD_clear(LCD l) {
	l.cursor_pos = LCD_LINE_1;
	LCD_sendcmd(l, 0x1);
	_delay_us(LCD_LONG_DELAY);
	LCD_sendcmd(l, l.cursor_pos);
}

LCD LCD_putchar(LCD l, uint8_t ch) {
	*(l.cmd_port) |= 1 << LCD_RS_BIT;
	*(l.cmd_port) &= ~(1 << LCD_RW_BIT); // RW = 0
	*(l.data_port) = ch;
	*(l.cmd_port) |= 1 << LCD_EN_BIT;

	_delay_us(LCD_DELAY);
	
	*(l.cmd_port) &= ~(1<<LCD_EN_BIT);
	
	_delay_us(LCD_MINI_DELAY);
	
	if (ch == '\n') {
		if (l.cursor_pos >= LCD_LINE_1 && l.cursor_pos < LCD_LINE_1+l.char_num) {
			l.cursor_pos = LCD_LINE_2;
			LCD_sendcmd(l, LCD_LINE_2);
		} else if (l.cursor_pos >= LCD_LINE_2 && l.cursor_pos < LCD_LINE_2+l.char_num) {
			l.cursor_pos = LCD_LINE_3;
			LCD_sendcmd(l, LCD_LINE_3);
		} else if (l.cursor_pos >= LCD_LINE_3 && l.cursor_pos < LCD_LINE_3+l.char_num) {
			l.cursor_pos = LCD_LINE_4;
			LCD_sendcmd(l, LCD_LINE_4);
		} else if (l.cursor_pos >= LCD_LINE_4 && l.cursor_pos < LCD_LINE_4+l.char_num) {
			l.cursor_pos = LCD_LINE_1;
			LCD_sendcmd(l, LCD_LINE_1);
		}
	} else {
		if (l.cursor_pos == LCD_LINE_1 + l.char_num-1) {
			l.cursor_pos = LCD_LINE_2;
			LCD_sendcmd(l, LCD_LINE_2);
		} else if (l.cursor_pos == LCD_LINE_2 + l.char_num-1) {
			l.cursor_pos = LCD_LINE_3;
			LCD_sendcmd(l, LCD_LINE_3);
		} else if (l.cursor_pos == LCD_LINE_3 + l.char_num-1) {
			l.cursor_pos = LCD_LINE_4;
			LCD_sendcmd(l, LCD_LINE_4);
		} else if (l.cursor_pos == LCD_LINE_4 + l.char_num-1) {
			l.cursor_pos = LCD_LINE_1;
			LCD_sendcmd(l, LCD_LINE_1);
		} else {
			l.cursor_pos++;
		}
	}
		
	return l;
}

void LCD_print(LCD l, uint8_t str[]) {
	for (uint8_t i = 0; str[i] != '\0'; i++)
	l = LCD_putchar(l, str[i]);
}

#endif
