; Stuff needs to be done implement 

; 16bit comparison to check if pointer is bigger 
; then SRAM SIZE set it so RAM_START in save_log:

; Request button needs to be implemented after main:

.include "m128def.inc"

.equ TEMPERATURE=31
.equ MOISTURE=32
.equ WATER_LEVEL=33

; Initialization of stuff

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
sts ddrf, r16 ; PORT F is I/0 extended we can't use out it's out of range.

; Settings up port G
; pin1 > pin0
; ldi r16, 0b11100 
; sts ddrg, r16
; ldi r16, 0b00011
; sts ping, r16

; Setting pointer for logging.
ldi xl, LOW(SRAM_START)
ldi xh, HIGH(SRAM_START)

jmp main

; It might come weird at first but since we dont have <= in AVR we have to use >=
; r17 is sensor value
; r18 is min value
; r19 is max value
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

; r17 is sensor value
; X is pointer in memory.
save_log:
	; CHECK IF X > RAMEND if so set X to SRAM_START
	st x+, r17
	ret

main:
	; check for request signal
	; lds r16, ping
	; cpi r16, 0xFF
	; breq accept_request
	; rjmp main
	accept_request:
		;sts portg, r16 ; Light up acknowledge led.
		
		; Temperature
		; in r17, pina ; temp value
		ldi r17, TEMPERATURE
		ldi r18, 10 ; min temp
		ldi r19, 240 ; max temp
		rcall validate_range ; validate range: if valid keep value if not set r17 = 0xFF
		rcall save_log
		out portd, r17 ; displaying result

		; Moisture
		; in r17, pinb ; moisture value
		ldi r17, MOISTURE
		ldi r18, 20 ; min moisture
		ldi r19, 200 ; max moisture
		rcall validate_range ; validate range: if valid keep value if not set r17 = 0xFF
		rcall save_log
		out porte, r17 ; displaying result

		; Water level
		; in r17, pinc ; water level value
		ldi r17, WATER_LEVEL
		ldi r18, 5 ; min water level
		ldi r19, 250 ; max water level
		rcall validate_range ; validate range: if valid keep value if not set r17 = 0xFF
		rcall save_log
		sts portf, r17 ; displaying result

		ldi r17, 0x00
		rcall save_log

		rjmp main