#define F_CPU 8e6

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#include "./user_interface.h"
#include "./terminal.h"
#include "./timer.h"

// Defining buffer sizes.
#define USER_OUTPUT_BUFFER_SIZE 128
#define USER_INPUT_BUFFER_SIZE 4
#define CHECK_PIN(port, pin) ( (port & (1 << pin)) >> pin)


// Defining user input buffer.
Buffer *userInputBuffer;

// Defining user output buffer.
Buffer *userOutputBuffer;

// Defining memory pointers.
uint8_t *x = (uint8_t *) 0x0500;
uint8_t *iterator = (uint8_t *) 0x0500;
uint8_t *data_start = (uint8_t *) 0x0500;

uint8_t mcu_watchdog = 0;
uint8_t sensor_watchdog = 0;

bool shouldReadSlave = true;
bool shouldReadMaster = true;
// Defining memory dump flag this is needed
// because of memory content will not fit
// into buffer.
bool memoryDumpActive = false;

void startTimer() {
	switch(sensor_watchdog) {
		case 1:
			setupTimer1(CLKS_0_5);
			break;
		case 2:
			setupTimer1(CLKS_1_0);
			break;
		case 3:
			setupTimer1(CLKS_2_0);
			break;
	}
}

// Enables transmit and disables receive.
void enableTransmit() {
	UCSR0B &= ~((1 << RXEN0) | (1 << RXCIE0)); // Disable receive.
	UCSR0B |= (1 << TXCIE0) | (1 << TXEN0); // Enable transmit.
}

// Checks if there is config in EEPROM.
bool isConfigSet(uint8_t* config) {
	return !(config[0] == 0xFF || config[1] == 0xFF || 
			config[2] == 0xFF || config[3] == 0xFF);
}

// Configures user interface.
void configureUserInterface() {
	UBRR0L = 51; // Sets BAUD rate to 9600 for 8MHz.
	
	userInputBuffer = newBuffer(USER_INPUT_BUFFER_SIZE);
	userOutputBuffer = newBuffer(USER_OUTPUT_BUFFER_SIZE);
	uint8_t config[4];
	
	eeprom_busy_wait();
	// array, address, size
	eeprom_read_block(config, (uint8_t *) 0x01, sizeof(config));
	
	if (!isConfigSet(config)) {
		sendUserMessageAsync("There is no config.\n");
		shouldReadMaster = true;
		shouldReadSlave = true;
		sendUserMessageAsync("\nEnter MS WD Choice (& period):\n1) 30ms\n2) 250ms\n3) 500ms\n");

	} else {
		sendUserMessageAsync("Config found \n");
		mcu_watchdog = config[0];
		sensor_watchdog = config[2];
		shouldReadSlave = false;
		shouldReadMaster = false;
		startTimer();
		displayMenu();
		resetRequest();
	};
	
}


// Enables receive and disables transmit.
void enableRecieve() {
	UCSR0B &= ~((1 << TXCIE0) | (1 << TXEN0)); // Disable transmit.
	UCSR0B |= (1 << RXCIE0) | (1 << RXEN0); // Enable receive.
}

// Appends message to user output buffer to be displayed.
void sendUserMessageAsync(const char *message) {
	bufferWrite(userOutputBuffer, message); // Appends to message output buffer.
	if (!userOutputBuffer->writeActive) {
		userOutputBuffer->writeActive = true;
		enableTransmit();
		UDR0 = outputWrite(userOutputBuffer); // Triggers transaction.
	}
}

// Formats given message into format of address:value where address and value are in hex.
void formatMemoryEntry(uint8_t *x, char *message) {
	sprintf(message, "0x%.4X:0x%.2X\n", x, *x);
}

void displayMenu() {
	sendUserMessageAsync("1) Mem dump\n2) Last entry: ");
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
	if (x == data_start) {
		sendUserMessageAsync("No entry exists.\n");
	} else {
		formatMemoryEntry(x - 1, message);
		sendUserMessageAsync(message);
	}
	sendUserMessageAsync("\n");
	// displayMenu();
}

void printInvalidSelection() {
	sendUserMessageAsync("Invalid selection\n");
	displayMenu();
}

void handleUserInput() {
	int selection = atoi(userInputBuffer->buffer);
	if (!shouldReadSlave && !shouldReadMaster) {
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
	} else {
		if (shouldReadMaster) {
			sendUserMessageAsync("\nEnter Slave WD Choice (& period):\n1) 0.5s\n2) 1s\n3) 2s\n");
			shouldReadMaster = false;
			mcu_watchdog = selection;
			
			eeprom_busy_wait();
			// 0x01 0x02
			// address, value
			eeprom_write_byte((uint8_t *) 0x01, mcu_watchdog);
			eeprom_write_byte((uint8_t *) 0x02, 0);
			
		} else if (shouldReadSlave) {
			
			sensor_watchdog = selection;
			eeprom_busy_wait();
			// 0x01 0x02
			eeprom_write_byte((uint8_t *) 0x03, sensor_watchdog);
			eeprom_write_byte((uint8_t *) 0x04, 0);
			shouldReadSlave = false;

			startTimer();
			displayMenu();
		}
	}
}

ISR(USART0_RX_vect) {
	char input = UDR0; // Saves user input to buffer.
	inputHandler(input, userInputBuffer, handleUserInput);
};

void handleUserOutputFinished() {
	if (!memoryDumpActive) {
		userOutputBuffer -> writeActive = false;
		clearOutputBuffer(userOutputBuffer);
		if (!USER_INTERFACE_DEBUG) {
			enableRecieve();
		}
	} else {
		// While our iterator is smaller than x
		if (iterator < x) {

			clearOutputBuffer(userOutputBuffer);
			// Adds next memory entry into buffer and increment iterator.
			char message[16];
			formatMemoryEntry(iterator, message);
			bufferWrite(userOutputBuffer, message);

			iterator++;

			if (iterator > (uint8_t * )(0x10FF - 50) && iterator < (uint8_t * ) 0x10FF) {
				iterator = (uint8_t * ) 0x1100;
			}
			
			UDR0 = outputWrite(userOutputBuffer); // Triggers transaction.
		}
		// We finished iterating over all iterators
		else {
			userOutputBuffer -> writeActive = false;
			iterator = (uint8_t * ) 0x0500; // Reset iterator
			memoryDumpActive = false; // Disable memory dump.

			sendUserMessageAsync("\n");
			displayMenu();
		}
	}
}

void handleUserOutputRead() {
	UDR0 = bufferCursor(userOutputBuffer);
}

ISR(USART0_TX_vect) {
	outputHandler(userOutputBuffer, handleUserOutputRead, handleUserOutputFinished);
};