.include "m128def.inc"

.EQU STACK_PADDING = 40
; CRC Polynomial
.EQU CRC_KEY = 0b11010100; 0b    in actuality, padded for convenience

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
.EQU PACKET_IN=pinb
.EQU CONTROL_SWITCH=pinf
.EQU RECIEVE_BTN=pinf
.EQU READY_LED=portf

; .EQU RAM_DATA_PORT = 
.DEF PACKET_IN_REG=r25
.DEF TOS=r23
.DEF IGNORE_REG=r24

; Macros

; Setting inputs and outputs
.macro CONFIGURE_PORTS
  ; Control switch G[0:2] 
  ; Recieve button G[3]
  ; Ready G[4]
  ldi r16, 0b0010000
  sts ddrf, r16

  ; Readout B[0:7]
  ldi r16, 0xFF
  out ddre, r16

  ; Packets IN C[0:7]
  ldi r16, 0x00
  out ddrb, r16

  ; Packets OUT D[0:7]
  ldi r16, 0xFF
  out ddrd, r16
.endmacro

.macro CONFIGURE_XMEM
  lds r16, MCUCR ;
  ori r16, 0b10000000
  sts MCUCR, r16 ; enable XMEM
  ldi r16, 0;
  sts XMCRA, r16
  sts XMCRB, r16
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

.cseg
.org 0x0000
rjmp configure

.org 0x0100

; Helper functions

; registers used: r16-r20
; r16 = low byte of input (padded with 5 zeros)
; r17 = high byte of input
crc11_generate:
  ldi r18, CRC_KEY
  ldi r19, 0 ; counter
  eor r17, r18

crc11_generate_loop_start:
  cpi r19, 16-5;
  brsh end_crc11_generate;

  mov r20, r17; copy to check
  andi r20, 0b10000000;
  breq crc11_generate_loop_roll_left; branch if leading zero aka Z=1

  ; else crc = crc xor poly
  eor r17, r18;
  rjmp crc11_generate_loop_start;

crc11_generate_loop_roll_left:
  inc r19; increment counter

  rol r16; r16 msb to Carry
  rol r17; Carry to lsb of r17

  rjmp crc11_generate_loop_start;


end_crc11_generate:
  ; r17 5 msb contains the crc code
  lsr r17;
  lsr r17;
  lsr r17;
  ret;


; input r16 = xxx00000
; output r17 = xxx[CRC-code, 5 bits]
; regs_used = r16, r17, r18
crc3:
  mov r18, r16; keep the original in r18
  ldi r17, 0; high byte == 0
  rcall crc11_generate; crc code will be in r17
  or r17, r18;
  ret;


; input r16=low byte with xxxccccc
; input r17=high byte with xxxxxxxx
; output r17 = 0 if no data corruption, otherwise there is data corruption
crc11_check:
  rcall crc11_generate;
  ret;


; crc3_check
; input=r16
; output = r17 = 0 if no data corruption, otherwise there is data corruption
crc3_check:
  ldi r17, 0x00 ; high byte == 0
  rcall crc11_generate ; crc code will be in r17
  ret;

init:
  ldi r16, RESET_REQUEST ; r16 = xxx0 0000
  rcall crc3 ; r18 = xxxy yyyy        
  rcall transmit_packet
  ret


service_readout:
  ret

; is_stack_empty() : (r18 = boolean)
is_stack_empty:
  push r16
  push r17
  in r16, spl
  in r17, sph
  ldi r18, low(RAMEND) - 4
  ldi r19, high(RAMEND)
  cp r16, r18
  cpc r17, r19
  breq is_stack_empty_pass
  ldi r18, 0
  rjmp is_stack_empty_end
is_stack_empty_pass:
  ldi r18, 1
is_stack_empty_end:
  pop r17
  pop r16
  ret

inc_x:
  push r16
  push r17
  ldi r16, low(RAMEND) - STACK_PADDING
  ldi r17, high(RAMEND)
  ; (RAMEND - 40) < x
  cp r16, xl
  cpc r17, xh
  brlo bigger
  rjmp inc_x_end
bigger:
  ldi xl, low(SRAM_START)
  ldi xh, high(SRAM_START)
inc_x_end:
  adiw xh:xl, 1
  ret

; transmit_pack(r17)
; PACKET_OUT = r17
transmit_packet:
  out PACKET_OUT, r17
  ret


; CODE starts here.

configure:
  CONFIGURE_PORTS
  CONFIGURE_XMEM
  CONFIGURE_POINTER

main:
  INIT_STACK
  rcall init
  push r17 ; TOS = PACKET_OUT

start_readout: 
  rcall service_readout

  ; Start == 1 ?
  lds r16, CONTROL_SWITCH ; yyyy yxxx
  andi r16, 0b00000111 ; r16 = 0000 0xxx.
  cpi r16, 1
  brne main ; R16 is not equal to 1 jump to main.

  ldi r16, 0x10
  sts READY_LED, r16 ; Set READY on.

  lds r16, RECIEVE_BTN
  sbrs r16, 3
  rjmp start_readout ; Skip if RECEIVE_BTN == 1.

recieve_check:
  lds r16, RECIEVE_BTN
  sbrc r16, 3
  rjmp recieve_check ; Skip if RECIEVE_BTN == 0

  ldi r16, 0x00
  sts READY_LED, r16; Set READY off.

  in PACKET_IN_REG, PACKET_IN 

  sbrc PACKET_IN_REG, 7 
  rjmp not_command_type ; Skip if PACKET_IN is control type.
  rjmp tos_has_data

not_command_type:
  rcall is_stack_empty
  sbrs r18, 0
  pop IGNORE_REG ; Skip if stack is empty.
  push PACKET_IN_REG
  rjmp start_readout

tos_has_data:

  ; Notice it doesn't say pop stack it just say look up TOS
  pop TOS
  push TOS

  sbrc TOS, 7
  rjmp check11 ; JUMP IF TOS IS DATA TYPE
  rjmp check3 ; JUMP IF TOS IS CONTROL TYPE
check11:
  ; crc11_check(r17 = data, r16 = control) : (r17 = 0 if no errors)
  mov r16, PACKET_IN_REG
  mov r17, TOS
  rcall crc11_check
  cpi r17, 0
  ;check11_pass:
  breq state_log_request
  ;check11_fail:
  pop IGNORE_REG; random reg
  rcall repeat_request
  rjmp start_readout

repeat_request:
  ldi r16, ERROR_REPEAT
  rcall crc3; CRC3 input=r16 output in r17
  rcall transmit_packet
  ret

state_log_request:
  andi PACKET_IN_REG, 0b11100000 ; xxx0 0000
  cpi PACKET_IN_REG, LOG_REQUEST
  brne main

  ; Save TOS to memory
  pop TOS
  st x, TOS ; CUSTOM LOGIC 
  rcall inc_x
  ; TOS = ACKNOWLEDGE
  ldi r16, ACKNOWLEDGE
  rcall crc3
  push r17

  rcall transmit_packet
  rjmp start_readout

check3:
  mov r16, PACKET_IN_REG
  rcall crc3_check; 
  cpi r17, 0
  breq check3_pass
  ;check3_fail:
  rcall repeat_request
  rjmp start_readout
check3_pass:
  andi PACKET_IN_REG, 0b11100000 ; xxx0 0000
  cpi PACKET_IN_REG, ACKNOWLEDGE
  brne state_acknowledge_fail
  ;state_acknowledge_pass:
  rcall is_stack_empty
  sbrc r18, 0
  ;stack_is_empty_fail:
  rjmp start_readout ; Skip if stack is NOT empty.
  pop r18
  ;stack_is_empty_pass:
  rjmp start_readout

state_acknowledge_fail:
  ;repeat:
  andi PACKET_IN_REG, 0b11100000 ; xxx0 0000
  cpi PACKET_IN_REG, ERROR_REPEAT
  brne repeat_fail
  ; Doesn't reach so i had to do this way.

  ;repeat_pass
  rcall is_stack_empty
  sbrc r18, 0
  ;stack_is_empty_pass:
  rjmp start_readout
  ;stack_is_empty_fail:
  pop r17
  out PACKET_OUT, r17
  rjmp start_readout

repeat_fail:
  rjmp start_readout
