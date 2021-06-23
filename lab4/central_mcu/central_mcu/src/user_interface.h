#ifndef USER_INTERFACE_H_
#define USER_INTERFACE_H_
#define USER_INTERFACE_DEBUG 0

#include <stdint.h>

extern uint8_t *x;
extern uint8_t *data_start;
extern uint8_t mcu_watchdog;
extern uint8_t sensor_watchdog;

void configureUserInterface();
void displayMenu();
void sendUserMessageAsync(const char *message);

#endif