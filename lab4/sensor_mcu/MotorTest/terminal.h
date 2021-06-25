#ifndef TERMINAL_H_
#define TERMINAL_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	uint8_t index;
	uint8_t size;
	char *buffer;
	bool writeActive;
} Buffer;

Buffer *newBuffer(uint8_t size);
void inputHandler(char input, Buffer *buffer, void handler());
void outputHandler(Buffer *buffer, void read(), void finished());
char outputWrite(Buffer *buffer);
char bufferCursor(Buffer *buffer);
void bufferWrite(Buffer *buffer, const char *message);
void clearOutputBuffer(Buffer *buffer);


#endif