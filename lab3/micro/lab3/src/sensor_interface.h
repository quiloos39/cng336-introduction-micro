#ifndef SENSOR_INTERFACE_H
#define SENSOR_INFERFACE_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "./crc.h"

#define ACK_PACKAGE (1 << 6)
#define RESET_IDX 0
#define LOG_REQUEST_IDX 1
#define ACKNOWLEDGE_IDX 2
#define ERROR_REPEAT_IDX 3

// Defining sensor buffer size.
#define SENSOR_INPUT_BUFFER_SIZE 6
#define SENSOR_OUTPUT_BUFFER_SIZE 32

// Defining sensor input buffer.
char sensor_input_buffer[SENSOR_INPUT_BUFFER_SIZE];
uint8_t sensor_input_buffer_index = 0;

// Defining sensor output buffer.
char sensor_output_buffer[SENSOR_OUTPUT_BUFFER_SIZE];
uint8_t sensor_output_buffer_index = 0;

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
	UBRR1L = 51;
}

void transmitPacket(uint8_t packet) {
	enableSensorTransmit();
	char message[6 + 1];
    sprintf(message, "> 0x%.2X\r\n", packet);
    strcat(sensor_output_buffer, message);
    UDR1 = sensor_output_buffer[sensor_output_buffer_index++];
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
	// FILL IN
}

uint8_t convertBufferToPacket(char* buf) {
    // convert buffer into uint_8 packet
    char c1 = buf[2];
    char c2 = buf[3];

    uint8_t d1, d2;
    if (c1 >= 48 && c1 <= 57) { // c1 is digit
        d1 = c1 - 48;
    } else if (c1 >= 65 && c1 <= 90) { // uppercase
        d1 = (c1 - 65) + 10;
    } else if (c1 >= 97 && c1 <= 122) { // 
        d1 = (c1 - 97) + 10;
    }

    if (c2 >= 48 && c2 <= 57) { // c1 is digit
        d2 = c2 - 48;
    } else if (c2 >= 65 && c2 <= 90) { // uppercase
        d2 = (c2 - 65) + 10;
    } else if (c2 >= 97 && c2 <= 122) { // 
        d2 = (c2 - 97) + 10;
    }

    return (d1 << 4) | d2;
}



void handleRequest() {
	
    sensor_input_buffer[sensor_input_buffer_index] = '\0';
    uint8_t packet_in = convertBufferToPacket(sensor_input_buffer);
	
	char message[32];
	sprintf(message, "Packet received with value %.2X.\r\n", packet_in);
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
        if (crc11Check(last_packet, packet_in)) {
            if (commandType(packet_in) == LOG_REQUEST_IDX) { // if log request
                logData(last_packet); // last_packet has data.
                transmitPacket(ACK_PACKAGE | crc3(ACK_PACKAGE)); // transmit ack
            }	
		} 
        // CRC11 failed.
        else {
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
                    transmitPacket( (last_packet | crc3(last_packet)) );
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

void clearSensorOutputBuffer() {
    for (uint8_t i = 0; i < SENSOR_OUTPUT_BUFFER_SIZE;  i++) {
        sensor_output_buffer[i] = 0;
    }
    sensor_output_buffer_index = 0;
}

ISR(USART1_RX_vect) {
    char userInput = UDR1; // Saves user input to buffer.
	if (userInput == '\r') {
       handleRequest();
	   sensor_input_buffer_index = 0;
    } else {
        sensor_input_buffer[sensor_input_buffer_index] = userInput;
        sensor_input_buffer_index = (sensor_input_buffer_index + 1) % SENSOR_INPUT_BUFFER_SIZE;
	}
};

ISR(USART1_TX_vect) {
    // End of stream
    if (sensor_output_buffer[sensor_output_buffer_index] == '\0') {
        clearSensorOutputBuffer();
		enableSensorReceive();
	}
    // There is still data in data stream.
    else {
        UDR1 = sensor_output_buffer[sensor_output_buffer_index++];
    }
}

#endif