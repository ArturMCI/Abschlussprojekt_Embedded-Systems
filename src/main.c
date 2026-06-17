#include "stm32f0xx.h"
#include "uart.h"
#include "protocol.h"
#include "led_matrix.h"

int main(void)
{
    uart_init();
    led_matrix_init();

    while(1)
    {
        protocol_receive_frame();
    }
}