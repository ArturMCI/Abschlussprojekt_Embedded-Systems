#include "stm32f0xx.h"
#include "uart.h"
#include "protocol.h"
#include "field.h"

int main(void)
{
    uart_init();

    while (1)
    {
        protocol_receive_frame();
    }
}