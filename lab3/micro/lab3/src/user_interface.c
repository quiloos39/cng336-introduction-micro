#include "./user_interface.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Defining buffer sizes.
#define USER_OUTPUT_BUFFER_SIZE 128
#define USER_INPUT_BUFFER_SIZE 4

// Defining user input buffer.
char userInputBuffer[USER_INPUT_BUFFER_SIZE];
uint8_t userInputBufferIndex = 0;

// Defining user output buffer.
char userOutputBuffer[USER_OUTPUT_BUFFER_SIZE];
uint8_t userOutputBufferIndex = 0;
bool userOutputBufferActive = false;

// Defining memory pointers.
uint8_t *x = (uint8_t *) 0x0590;
uint8_t *iterator = (uint8_t *) 0x0500;

// Defining memory dump flag this is needed
// because of memory content will not fit
// into buffer.
bool memoryDumpActive = false;

// Enables transmit and disables receive.
void enableTransmit() {
	UCSR0B &= ~((1 << RXEN0) | (1 << RXCIE0)); // Disable receive.
	UCSR0B |= (1 << TXCIE0) | (1 << TXEN0); // Enable transmit.
}

// Configures user interface.
void configureUserInterface() {
	enableTransmit();
	UBRR0L = 51;
}

// Enables receive and disables transmit.
void enableRecieve() {
	UCSR0B &= ~((1 << TXCIE0) | (1 << TXEN0)); // Disable transmit.
	UCSR0B |= (1 << RXCIE0) | (1 << RXEN0); // Enable receive.
	userOutputBufferActive = false;
}

// Clears user input buffer.
void clearUserInputBuffer() {
	for (uint8_t i = 0; i < USER_INPUT_BUFFER_SIZE; i++) {
		userInputBuffer[i] = 0;
	}
	userInputBufferIndex = 0;
}

// Clears user output buffer.
void clearUserOutputBuffer() {
	for (uint8_t i = 0; i < USER_OUTPUT_BUFFER_SIZE; i++) {
		userOutputBuffer[i] = 0;
	}
	userOutputBufferIndex = 0;
}

// Appends message to user output buffer to be displayed.
void sendUserMessageAsync(const char *message) {
	strcat(userOutputBuffer, message); // Appends to message output buffer.
	if (!userOutputBufferActive) {
		enableTransmit();
		userOutputBufferActive = true;
		UDR0 = userOutputBuffer[userOutputBufferIndex++]; // Triggers transaction.
	}
}

// Formats given message into format of address:value where address and value are in hex.
void formatMemoryEntry(uint8_t *x, char *message) {
	sprintf(message, "0x%.4X:0x%.2X\n\r", x, *x);
}

void displayMenu() {
	sendUserMessageAsync(
	"1) Memory dump\n\r"\
	"2) Last entry\n\r"\
	"\n\r"\
	"Select your option: "
	);
}

void printMemoryDump() {
	memoryDumpActive = true;
	char message[16];
	formatMemoryEntry(iterator, message);
	iterator++;
	sendUserMessageAsync(message);
}

void printLastEntry() {
	char message[16];
	formatMemoryEntry(x, message);
	sendUserMessageAsync(message);
	sendUserMessageAsync("\n\r");
	displayMenu();
}

void printInvalidSelection() {
	sendUserMessageAsync("\n\rInvalid selection\n\r\n\r");
	displayMenu();
}

ISR(USART0_RX_vect) {
	char userInput = UDR0; // Saves user input to buffer.
	if (userInput == '\r') { // Gets trigerred if enter is pressed.
		userInputBuffer[userInputBufferIndex] = '\0';
		int selection = atoi(userInputBuffer); // Converts user input buffer into int.
		switch(selection) {
			case 1:
				printMemoryDump();
				break;
			case 2:
				printLastEntry();
				break;
			default:
				printInvalidSelection();
				break;
		}
		clearUserInputBuffer();
		} else {
			userInputBuffer[userInputBufferIndex] = userInput;
			userInputBufferIndex++;
			if (userInputBufferIndex > USER_INPUT_BUFFER_SIZE) {
				clearUserInputBuffer();
			}
	}
};

ISR(USART0_TX_vect) {
	// End of stream.
	if (userOutputBuffer[userOutputBufferIndex] == '\0') {
		// Normal mode uses buffer.
		if (!memoryDumpActive) {
			// Clearing buffer.
			clearUserOutputBuffer();
			enableRecieve();
		}
		// MemoryDump mode uses buffer but in a fancy way.
		// Reason we have MemoryDump mode and Normal mode is
		// we can't fit all string pairs into buffer so we have
		// to be clever about it.
		else {
			// While our iterator is smaller than x
			if (iterator <= x) {
				// Clear first 16 bytes of output buffer.
				for (uint8_t i = 0; i < 16; i++) {
					userOutputBuffer[i] = 0;
				}
				userOutputBufferIndex = 0;
				
				// Adds next memory entry into buffer and increment iterator.
				char message[16];
				formatMemoryEntry(iterator, message);
				strcat(userOutputBuffer, message);
				
				if (iterator > (0x10FF - 50) && iterator < 0x10FF) {
					iterator = 0x1100;
				}
				
				iterator++;
				
				UDR0 = userOutputBuffer[userOutputBufferIndex++]; // Triggers transaction.
			}
			// We finished iterating over all iterators
			else {
				enableRecieve();
				iterator = (uint8_t *) 0x0500; // Reset iterator
				memoryDumpActive = false; // Disable memory dump.
				sendUserMessageAsync("\n\r");
				displayMenu();
			}
		}
	}
	// There is still data in data stream.
	else {
		UDR0 = userOutputBuffer[userOutputBufferIndex++];
	}
	
};