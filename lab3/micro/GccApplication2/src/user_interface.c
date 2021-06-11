#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <avr/eeprom.h>

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
	char msg[16];
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

bool isConfigSet(uint8_t* config) {
	return !(config[0] == 0xFF || config[1] == 0xFF || 
			config[2] == 0xFF || config[3] == 0xFF);
}

// Configures user interface.
void configureUserInterface() {
	UBRR0L = 51;
	
	userInputBuffer = newBuffer(USER_INPUT_BUFFER_SIZE);
	userOutputBuffer = newBuffer(USER_OUTPUT_BUFFER_SIZE);
	uint8_t config[4];
	
	eeprom_busy_wait();
	eeprom_read_block(config, (uint8_t *) 0x01, sizeof(config));
	
	if (!isConfigSet(config)) {
		sendUserMessageAsync("There is no config.\r\n");
		shouldReadMaster = true;
		shouldReadSlave = true;
		sendUserMessageAsync("\rEnter MS WD Choice (& period):\r1) 30ms\r2) 250ms\r3) 500ms\r\n");

	} else {
		sendUserMessageAsync("There is config \r\n");
		mcu_watchdog = config[0];
		sensor_watchdog = config[2];
		shouldReadSlave = false;
		shouldReadMaster = false;
		
		startTimer();
		displayMenu();
	};
	
}


// Enables receive and disables transmit.
void enableRecieve() {
	UCSR0B &= ~((1 << TXCIE0) | (1 << TXEN0)); // Disable transmit.
	UCSR0B |= (1 << RXCIE0) | (1 << RXEN0); // Enable receive.
}

void sendUserMessageSync(const char *message) { // send a message (synchronously)
	uint8_t tmp = UCSR0B;
	
	UCSR0B = (1 << TXEN0);
	for (uint8_t i = 0 ; i < strlen(message); i++) {
		UDR0 = message[i];
		while (!CHECK_PIN(UCSR0A, TXC0));
		UCSR0A |= (1 << TXC0);
	}

	UCSR0B = tmp;
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
	sprintf(message, "0x%.4X:0x%.2X\r\n", x, *x);
}

void displayMenu() {
	sendUserMessageAsync(
	"1) Memory dump\r\n"\
	"2) Last entry\r\n"\
	"\r\n"\
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
	if (x == data_start) {
		sendUserMessageAsync("No entry exists.\r\n");
	} else {
		formatMemoryEntry(x - 1, message);
		sendUserMessageAsync(message);
	}

	sendUserMessageAsync("\r\n");
	displayMenu();
}

void printInvalidSelection() {
	sendUserMessageAsync("\r\nInvalid selection\r\n\r\n");
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
			sendUserMessageAsync("\rEnter Slave WD Choice (& period):\r1) 0.5s\r2) 1s\r3) 2s\r\n");
			shouldReadMaster = false;
			mcu_watchdog = selection;
			
			eeprom_busy_wait();
			// 0x01 0x02
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
			sendUserMessageAsync("\r\n");
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