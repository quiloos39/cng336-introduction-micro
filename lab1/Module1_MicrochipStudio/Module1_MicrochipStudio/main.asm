.include "m128def.inc"

; Setting stack pointer (SP) value to RAMEND literally RAM_END end of SRAM.
; SP, is stack pointer used for subroutines which are basically local
; function that, after done executing code block, continues where its has need called from.
ldi r16, LOW(RAMEND)  ; r16 = 0xFF
out SPL, r16
ldi r16, HIGH(RAMEND) ; r16 = 0x10
out SPH, r16

; Setting input ports A,B,C
ldi r16, 0x00
out ddra, r16
out ddrb, r16
out ddrc, r16

;Settings output ports D,E,F
ldi r16, 0xFF
out ddrd, r16
out ddre, r16
sts ddrf, r16

; Settings up port G pin0 = output pin1 = input
ldi r16, 0x01
sts ddrg, r16

; Setting pointer for logging.
ldi xl, 0xFF
ldi xh, 0x10

rjmp main

; r17 -> sensor value
; r18 -> min value
; r19 -> max value
validate_range:
	cp r18, r17
	brsh invalid_range ; r18 (min value) >= r17 (sensor_value) it will branch to invalid_range.
	cp r17, r19
	brsh invalid_range ; r17 (sensor_value) >= r19 (max value) it will branch to invalid_range.
	rjmp end
invalid_range:
	ldi r17, 0xFF
end:
	ret ; will return to where it has been called from.

clear_ports:
	ldi r16, 0x00
	out portd, r16
	out porte, r16
	sts portf, r16
	ldi r16, 0b00
	sts portg, r16
	ret

;10FF max
;1000 pointer

;r17 -> sensor value   
save_log:
	adiw xh:xl, 1
	ldi r18, LOW(RAMEND)
	ldi r19, HIGH(RAMEND)
	cp r18, xl
	cpc r19, xh
	brlo reset_X
	rjmp save
reset_x:
	ldi xl, LOW(SRAM_START)
	ldi xh, HIGH(SRAM_START)
save:
	st X, r17
	ret

main:
	lds r16, ping ; read from G i only care about last 2 bits / pins
	ori r16, 0b11111101 ; bit0/pin0 is output so i dont care about it's value
	cpi r16, 0xFF ; bit1/pin1 will change according to button so i check if its 11
	breq accept_request
	rcall clear_ports ; clears leds makes them turnoff
	rjmp main
	
accept_request:
	sts portg, r16 ; Light up acknowledge led.

	; Temperature
	in r17, pina ; temp value
	ldi r18, 10 ; min temp
	ldi r19, 240 ; max temp
	rcall validate_range ; validate range: if valid keep value if not set r17 = 0xFF
	rcall save_log
	out portd, r17 ; displaying result

	; Moisture
	in r17, pinb ; moisture value
	ldi r18, 20 ; min moisture
	ldi r19, 200 ; max moisture
	rcall validate_range ; validate range: if valid keep value if not set r17 = 0xFF
	rcall save_log
	out porte, r17 ; displaying result

	; Water level
	in r17, pinc ; water level value
	ldi r18, 5 ; min water level
	ldi r19, 250 ; max water level
	rcall validate_range ; validate range: if valid keep value if not set r17 = 0xFF
	rcall save_log
	sts portf, r17 ; displaying result
       
	ldi r17, 0x00 ; write char 0 to memory
	rcall save_log
   
	rjmp main