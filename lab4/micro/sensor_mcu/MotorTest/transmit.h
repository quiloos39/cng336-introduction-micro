#ifndef TRANSMIT_H
#define TRANSMIT_H

#include "cfg.h"

#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>
#include "terminal.h"

volatile Buffer *output_buffer;
volatile uint8_t last_packet;
volatile Buffer *input_buffer;

uint8_t last_data_packet = 0;
uint8_t last_data_packet_idx = 0;
bool last_data_ack_received = true; // if true, can modify last_data_packet

void configureBuffers() {
	input_buffer = newBuffer(128);
	output_buffer = newBuffer(128);
	
	DDRE &= ~(1<<PINE0);
	DDRE |= (1<<PINE1);
	UCSR0B |= (1 << RXCIE0) | (1 << RXEN0);
	UCSR0B |= (1 << TXCIE0) | (1 << TXEN0) ;
}

void sensorOutputRead() {
	UDR0 = bufferCursor(output_buffer);
}

void sensorOutputFinished() {
	clearOutputBuffer(output_buffer);
}


void transmitPacket(uint8_t packet) {
	char message[16 + 1];
	sprintf(message, "0x%.2X\r\n", packet);
	bufferWrite(output_buffer, message);
	UDR0 = outputWrite(output_buffer);
}

void repeatRequest() {
	last_packet = 0;
	uint8_t msg = (0b011 << 5); // padded repeat command
	msg |= crc3(msg);
	transmitPacket(msg);
}

uint8_t isCommand(uint8_t packet) {
	return !(0b10000000 & packet);
}

uint8_t commandType(uint8_t packet) {
	if (!(packet &  (1 << 6)) && (packet & (1 << 5))) {
		return LOG_REQUEST_IDX;
	} else if ( (packet &  (1 << 6)) && !((packet & (1 << 5))) ) {
		return ACKNOWLEDGE_IDX;
	} else if ( (packet &  (1 << 6)) &&   (packet & (1 << 5))  ) {
		return ERROR_REPEAT_IDX;
	}
}


void handleSensorData() {

	uint8_t cmd_packet;
	uint8_t data_packet;
	uint8_t crc11_code;
	uint8_t packet_in = strtol(input_buffer -> buffer, NULL, 16);
	switch(commandType(packet_in)) {
		case RESET_IDX:
			//transmitPacket(ACK_PACKAGE | crc3(ACK_PACKAGE));
			break;
			
		case ACKNOWLEDGE_IDX:
			last_data_ack_received = true;
			last_data_packet = 0;
			last_data_packet_idx = 0;
			break;
			
		case ERROR_REPEAT_IDX:
			cmd_packet = LOG_REQUEST_PACKAGE;
			data_packet = convertDataPacket(last_data_packet_idx);
			crc11_code = crc11(last_data_packet, cmd_packet);

			cmd_packet |= crc11_code;
			transmitPacket(data_packet);
			transmitPacket(cmd_packet);

			break;
	}
}


ISR(USART0_RX_vect) {
	char input = UDR0; // Saves user input to buffer.
	inputHandler(input, input_buffer, handleSensorData);
}

ISR(USART0_TX_vect) {
	outputHandler(output_buffer, sensorOutputRead, sensorOutputFinished);
}



#endif