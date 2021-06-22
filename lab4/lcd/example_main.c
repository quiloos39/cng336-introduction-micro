/*
 * Lab4LCD.c
 *
 * Created: 19/06/2021 20:47:02
 * Author : Oscar
 */ 
#define F_CPU 8000000 // 8 MHz


#include <avr/io.h>
#include "lcd.h" 

int main(void)
{
	DDRA = 0xFF;
	DDRB = 0xFF;
	
	LCD l = LCD_init(20, 4, &PORTA, &PORTB);
	//LCD_clear(l);
	
	LCD_print(l, "Hello world broooooS0AAAAAAAA");
    /* Replace with your application code */
    while (1) 
    {	


    }
}