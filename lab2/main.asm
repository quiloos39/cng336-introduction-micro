.include "m128def.inc"

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

.macro SERVICE_READOUT
.endmacro

; CHECK_CONTROL (number, label_true, label_false)
.macro CHECK_CONTROL
    in r16, CONTROL_SWITCH
    andi r16, 0b00000111 ; r16 = 0000 0xxx
    cmpi r16, @0
    brne @1 ; r16 == number ? label_true : label_false
.endmacro

; CHECK_RECIEVE (label_true, label_false)
.macro CHECK_RECIEVE
	in r16, RECIEVE_BTN
	sbrc r16, 3 ; Skip if RECIEVE_BTN[3] == 0
	rjmp @1 ; jump label_false
    rjmp @0 ; jump label_true
.endmacro

; IF_JUMP (label_true, label_false)
.macro IF_JUMP
	sbrc r18, 0
	rjmp @1 ; jump label_false
    rjmp @0 ; jump label_true
.endmacro

; CHECK_COMMAND_TYPE (COMMAND = r17, label_true, label_false)
.macro CHECK_COMMAND_TYPE
    andi r17, 0b11100000 ; xxx0 0000
    cmpi r17, @0
    breq @1
    rjmp @2 
.endmacro

.macro IS_COMMAND_TYPE
    sbrc r16, 7
    rjmp @0 ; jump label_true
    rjmp @1 ; jump label_false
.endmacro

.cseg
.org 0x00
rjmp configure

.org 0x100

; Helper functions

; crc3_check(r16 = control): (r18 = boolean)
crc3_check:
    mov r18, r16
    andi r16, 0b11100000 ; xxx0 0000
    rcall crc3 ; r16 = crc3(control = r16)
    cp r16, r18
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

; Writes to MEM
write_mem:
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

; stack_empty() : (r18 = boolean)
stack_empty:
	ldi r18, 1
	ret

configure:
	CONFIGURE_PORTS
	CONFIGURE_XMEM
	CONFIGURE_POINTER

main:
	INIT_STACK
    rcall init
	rcall write_mem

start_readout: 
	rcall service_readout

	; if (r16 = control_pins) == 1
	; rjmp check_control_pass
	; else rjmp main
	CHECK_CONTROL 1, check_control_pass, main

check_control_pass:
	sbi READY_LED, 4 ; READY = ON

	; if (r16 = recieve_pin) == 1
	; rjmp check_recieve_one_pass
	; else rjmp start_readout
	CHECK_RECIEVE check_recieve_one_pass, start_readout

check_recieve_one_pass:

	; if (r16 = recieve_pin) == 0
	; rjmp check_recieve_two_pass
	; else rjmp check_recieve_one_pass
	CHECK_RECIEVE check_recieve_one_pass, check_recieve_two_pass
	
check_recieve_two_pass:
	cbi READY_LED, 4 ; Set ready off
	in r16, PACKET_IN  ; r16 = CAPTURE_PACKET_IN
	
	; if (r16 = PACKET_IN) == COMMAND_TYPE
	; rjmp is_command_type
	; else rjmp not_command_type
	IS_COMMAND_TYPE is_command_type, not_command_type

not_command_type:	
    rjmp start_readout

is_command_type:
    mov r16, r17 ; Copy command to right place
	pop r16

	; if (r16 = TOP) == DATA_TYPE
	; rjmp check11
	; else rjmp check3
	IS_COMMAND_TYPE check3, check11

check11:
	; crc11_check(r16 = data, r17 = control): (r18 = boolean)
    rcall crc11_check

	; if r18 == 1
	; rjmp state_log_request
	; else rjmp check11_fail
	IF_JUMP state_log_request, check11_fail

check11_fail:
    ; Pop TOS
    rcall repeat_request

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
	; crc3_check(r16 = control): (r18 = boolean)
	rcall crc3_check

	; if r18 == 1
	; rjmp state_acknowledge
	; else rjmp repeat_request
    IF_JUMP state_acknowledge, repeat_request

state_acknowledge:
	; if (r17 = control) == ACKNOWLEDGE
	; rjmp state_acknowledge_pass
	; else rjmp state_repeat
    CHECK_COMMAND_TYPE ACKNOWLEDGE, state_acknowledge_pass, state_repeat

state_acknowledge_pass:
	rjmp start_readout

state_repeat:
	rjmp transmit_packet
	


	


	 

