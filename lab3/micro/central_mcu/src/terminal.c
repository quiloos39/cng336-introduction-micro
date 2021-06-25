#include <stdlib.h>
#include <string.h>
#include "./terminal.h"

Buffer *newBuffer(uint8_t size) {
	Buffer *buffer = malloc(sizeof(Buffer));
	buffer->size = size;
	buffer->index = 0;
	buffer->buffer = malloc(sizeof(char) * (size + 1));
	return buffer;
}

void inputHandler(char input, Buffer *buffer, void handler()) {
	if (input == '\r') {
		buffer->buffer[buffer->index] = '\0';
		handler();
		buffer->index = 0;
		} else if (input == '\b') {
		if (buffer->index > 0) {
			buffer->index--;
		}
	} else {
		buffer->buffer[buffer->index] = input;
		buffer->index = (buffer->index + 1) % (buffer->size + 2);
	}
}

void clearOutputBuffer(Buffer *buffer) {
	for (uint8_t i = 0; i < buffer->size; i++) {
		buffer->buffer[i] = 0;
	}
	buffer->index = 0;
}

void outputHandler(Buffer *buffer, void read(), void finished()) {
	if (buffer->buffer[buffer->index] == '\0') {
		finished();
	} else {
		read();
		buffer->index++;
	}
}

void bufferWrite(Buffer *buffer, const char *message) {
	strcat(buffer->buffer + buffer->index, message);
}

char outputWrite(Buffer *buffer) {
	return buffer->buffer[buffer->index++];
}

char bufferCursor(Buffer *buffer) {
	return buffer->buffer[buffer->index];
}