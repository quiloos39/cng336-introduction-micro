.include "m128def.inc"

; Testing
.EQU DATA_TYPE=1 << 7
.EQU CONTROL_TYPE=0 << 7

.EQU POLY_KEY=0b000110
.EQU POLY_KEY_LEN=5

; Command headers
.EQU RESET_REQUEST=0b000 << 5
.EQU LOG_REQUEST=0b001 << 5
.EQU ACKNOWLEDGE=0b010 << 5
.EQU ERROR_REPEAT=0b011 << 5

; Inputs and outputs
.EQU PACKET_OUT=portd
.EQU PACKET_IN=pinc
.EQU CONTROL_SWITCH=pina
.EQU RECIEVE_BTN=pina
.EQU READY_LED=porta

; Macros

; Setting inputs and outputs
.macro CONFIGURE_PORTS
	; Control switch A[0:2] 
	cbi ddra, 0
	cbi ddra, 1
	cbi ddra, 2

	; Readout B[0:7]
	ldi r16, 0xFF
	out ddrb, r16

	; Packets IN C[0:7]
	ldi r16, 0x00
	out ddrc, r16

	; Packets OUT D[0:7]
	ldi r16, 0xFF
	out ddrd, r16

	;Recieve button A[3]
	cbi ddra, 3

	; Ready A[4]
	sbi ddra, 4
.endmacro

.macro CONFIGURE_XMEM
.endmacro

; Initializing X to SRAM_START
.macro CONFIGURE_POINTER
	ldi xl, low(SRAM_START)
	ldi xh, high(SRAM_START)
.endmacro

; Initializing stack pointer to SRAM END
.macro INIT_STACK
	ldi r16, low(RAMEND)
	out spl, r16
	ldi r16, high(RAMEND)
	out sph, r16
.endmacro

; CHECK_COMMAND_TYPE (COMMAND = r17, label_true, label_false)
.macro CHECK_COMMAND_TYPE
    andi r17, 0b11100000 ; xxx0 0000
    cpi r17, @0
    breq @1
    rjmp @2 
.endmacro

.cseg
.org 0x0000
rjmp configure

.org 0x0100

; Helper functions

crc3:
	ret
crc11:
	ret

; crc3_check(r17 = control): (r18 = boolean)
crc3_check:
    mov r18, r17
    andi r17, 0b11100000 ; xxx0 0000
    rcall crc3 ; r17 = crc3(control = r17)
    cp r17, r18
    breq crc3_check_true
    ldi r18, 0
    rjmp crc3_check_end
crc3_check_true:
    ldi r18, 1
crc3_check_end:
    ret

; crc11_check(r16 = data, r17 = control): (r18 = boolean)
crc11_check:
    mov r18, r17
    andi r17, 0b11100000
    rcall crc11 ; r17 = crc11(data = r16, control = r17)
    cp r17, r18
    breq crc11_check_true
    ldi r18, 0
    rjmp crc11_check_end
crc11_check_true:
    ldi r18, 1
crc11_check_end:
    ret

init:
	ldi r16, RESET_REQUEST
	rcall crc3
	out PACKET_OUT, r16
    ret

repeat_request:
    ret

service_readout:
    ret

; is_stack_empty() : (r18 = boolean)
is_stack_empty:
	ldi r18, 1
	ret

configure:
	CONFIGURE_PORTS
	CONFIGURE_XMEM
	CONFIGURE_POINTER

main:
	INIT_STACK
    rcall init
	push r16

start_readout: 
	rcall service_readout

	ldi r16, 1 ; Testing remove this after test.
	
	;in r16, CONTROL_SWITCH
    andi r16, 0b00000111 ; r16 = 0000 0xxx.
    cpi r16, 1
    brne main ; R16 is not equal to 1 jump to main.

	sbi READY_LED, 4 ; Set READY on.

	ldi r16, 0xFF ; Testing remove this after test.

	;in r16, RECIEVE_BTN
	sbrs r16, 3
    rjmp start_readout ; Skip if RECEIVE_BTN == 1.

recieve_check:

	;in r16, RECIEVE_BTN
	ldi r16, 0x00 ; Testing remove this after test.
	sbrc r16, 3
	rjmp recieve_check ; Skip if RECIEVE_BTN == 0

	cbi READY_LED, 4 ; Set READY off.
	; in r17, PACKET_IN  ; r16 = CAPTURE_PACKET_IN.
	ldi r17, CONTROL_TYPE ; Testing remove this after test.

	sbrc r17, 7 ; Skip if PACKET_IN is control type.
    rjmp not_command_type
    rjmp tos_has_data

not_command_type:
	rcall is_stack_empty
	sbrs r18, 0
	pop r18 ; Skip if stack is empty.
	push r17
    rjmp start_readout

tos_has_data:
	pop r16
	ldi r16, CONTROL_TYPE ; r16 = TOS.

	sbrc r16, 7
    rjmp check11 ; Skip if TOS is control type.
    rjmp check3

check11:
	; crc11_check(r16 = data, r17 = control): (r18 = boolean)
    rcall crc11_check

	sbrc r18, 0
	rjmp state_log_request ; Skip if crc11_check fails.
	pop r18
	rjmp repeat_request

state_log_request:
	; if (r17 = control) == LOG_REQUEST
	; rjmp state_log_request_pass
	; else rjmp main
    CHECK_COMMAND_TYPE LOG_REQUEST, state_log_request_pass, main

state_log_request_pass:
    rjmp transmit_packet
    
transmit_packet:
    rjmp start_readout


check3:
	; crc3_check(r17 = control): (r18 = boolean)
	; rcall crc3_check
	ldi r18, 1

	sbrs r18, 0 
	rjmp repeat_request ; Skip if crc3_check passes.

	ldi r17, ACKNOWLEDGE
	; if (r17 = control) == ACKNOWLEDGE
	; rjmp state_acknowledge_pass
	; else rjmp state_repeat
    CHECK_COMMAND_TYPE ACKNOWLEDGE, state_acknowledge_pass, state_acknowledge_fail

state_acknowledge_pass:
	rcall is_stack_empty
	sbrc r18, 0
	rjmp start_readout ; Skip if stack is NOT empty.
	pop r18
	rjmp start_readout

state_acknowledge_fail:
	CHECK_COMMAND_TYPE ERROR_REPEAT, state_repeat_pass, start_readout
	rjmp transmit_packet

state_repeat_pass:
	rcall is_stack_empty
	sbrc r18, 0
	rjmp start_readout
	pop r17
	out PACKET_OUT, r17
	rjmp start_readout