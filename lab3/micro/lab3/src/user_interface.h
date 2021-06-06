#ifndef USER_INTERFACE_H_
#define USER_INTERFACE_H_
#define USER_INTERFACE_DEBUG 1

#include <stdint.h>

extern uint8_t *x;

void configureUserInterface();
void displayMenu();
void sendUserMessageAsync(const char *message);

#endif