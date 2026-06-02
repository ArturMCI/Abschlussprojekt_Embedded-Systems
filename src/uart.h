#ifndef UART.H
#define UART.H

#include <stdint.h>

void uart_init(void);

void uart_send(uint8_t data);

uint8_t uart_receive(void);

#endif