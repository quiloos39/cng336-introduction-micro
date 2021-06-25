#ifndef SENSOR_INTERFACE_H
#define SENSOR_INFERFACE_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "./crc.h"
#include "./terminal.h"
#include "./timer.h"

#define ACK_PACKAGE (1 << 6)
#define RESET_IDX 0
#define LOG_REQUEST_IDX 1
#define ACKNOWLEDGE_IDX 2
#define ERROR_REPEAT_IDX 3
#define DATA_START 0x500

// Defining sensor buffer size.
#define SENSOR_INPUT_BUFFER_SIZE 6
#define SENSOR_OUTPUT_BUFFER_SIZE 32
#define DATA_INTERNAL_END (0x10ff - 50)
#define DATA_EXTERNAL_START 0x1100
#define DATA_EXTERNAL_END (0x1100 + 2000)

// Defining sensor input buffer.
Buffer *sensorInputBuffer;

// Defining sensor output buffer.
Buffer *sensorOutputBuffer;

// Last packet received.
uint8_t last_packet;

void enableSensorTransmit() {
	UCSR1B |= (1 << TXCIE1) | (1 << TXEN1) ;
	UCSR1B &= ~((1 << RXCIE1) | (1 << RXEN1));
}

void enableSensorReceive() {
	UCSR1B |= (1 << RXCIE1) | (1 << RXEN1);
	UCSR1B &= ~((1 << TXCIE1) | (1 << TXEN1));
}

void configureSensorInterface() {
	sensorInputBuffer = newBuffer(SENSOR_INPUT_BUFFER_SIZE);
	sensorOutputBuffer = newBuffer(SENSOR_OUTPUT_BUFFER_SIZE);
	UBRR1L = 51;
	enableSensorReceive();
}

void transmitPacket(uint8_t packet) {
	enableSensorTransmit();
	char message[16 + 1];
	sprintf(message, "> 0x%.2X\r\n", packet);
	bufferWrite(sensorOutputBuffer, message);
	UDR1 = outputWrite(sensorOutputBuffer);
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

void logData(uint8_t packet) {
	*x = packet;
	x++;

	if (x == (uint8_t *) DATA_INTERNAL_END) {
		x = (uint8_t *) DATA_EXTERNAL_START;
	} else if (x == (uint8_t *) DATA_EXTERNAL_END) {
		x = (uint8_t *) DATA_START;
	}
}

void resetRequest() {
	transmitPacket(0 | crc3(0)); // send reset package
}

// Sensor input handling.
// ----------------------

void handleSensorData() {
	resetTimer1();
	uint8_t packet_in = strtol(sensorInputBuffer -> buffer, NULL, 16);

	char message[32];
	sprintf(message, "Packet received %.2X\r\n", packet_in);
	sendUserMessageAsync(message);

	// Is PACKET_IN data type ?
	if (!isCommand(packet_in)) {
		sendUserMessageAsync("Received data packet.\r\n");
		last_packet = packet_in;
		return;
	}

	// PACKET_IN: command type, LAST_PACKET: data type.
	if (!isCommand(last_packet)) {
		// CRC11 pass.
		sendUserMessageAsync("Last packet is a data\r\n");
		if (crc11Check(last_packet, packet_in)) {
			if (commandType(packet_in) == LOG_REQUEST_IDX) { // if log request
				sendUserMessageAsync("Acknowledge\r\n");
				logData(last_packet); // last_packet has data.
				transmitPacket(ACK_PACKAGE | crc3(ACK_PACKAGE)); // transmit ack
			} else {
				sendUserMessageAsync("Invalid command.\r\n");
				resetRequest();
			}
		}
		// CRC11 failed.
		else {
			sendUserMessageAsync("CRC11 failed\r\n");
			repeatRequest();
			return;
		}
	}
	// PACKET_IN: command type: LAST_PACKET: command type.
	else {
		// CRC3 pass
		if (crc3Check(packet_in)) {
			// PACKET_IN is acknowledge command.
			if (commandType(packet_in) == ACKNOWLEDGE_IDX) {
				// debugUserInterface("Acknowledge received.\n\r");
				last_packet = 0;
			}
			// PACKET_IN is error repeat command.
			else if (commandType(packet_in) == ERROR_REPEAT_IDX) { // command not ack
				if (last_packet != 0) {
					transmitPacket((last_packet | crc3(last_packet)));
					last_packet = 0;
				}
			}
		}
		// CRC3 fail
		else {
			// debugUserInterface("Corrupted package sending repeat request. \r\n");
			repeatRequest();
		}
	}
}

ISR(USART1_RX_vect) {
	char input = UDR1; // Saves user input to buffer.
	inputHandler(input, sensorInputBuffer, handleSensorData);
};

// Sensor output handling.
// -----------------------

void handleSensorOutputFinished() {
	enableSensorReceive();
	clearOutputBuffer(sensorOutputBuffer);
}

void handleSensorOutputRead() {
	UDR1 = bufferCursor(sensorOutputBuffer);
}

ISR(USART1_TX_vect) {
	outputHandler(sensorOutputBuffer, handleSensorOutputRead, handleSensorOutputFinished);
}

#endif