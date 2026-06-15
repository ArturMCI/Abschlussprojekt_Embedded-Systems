#include "stm32f0xx.h";

#include "uart.h";

void uart_init(void) {
    // Clocks für GPIOA und USART2 aktivieren:
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    //PA2 und PA3 konfigurieren:
    GPIOA->MODER &= ~((0b0011 << 2 * 2) | (0b0011 << 3 * 2));
    GPIOA->MODER |= ((0b0010 << 2 * 2) | (0b0010 << 3 * 2));

    GPIOA->AFR[0] &= ~((1111 << 2 * 4) | (1111 << 3 * 4));
    GPIOA->AFR[0] |= ((1 << 2 * 4) | (1 << 3 * 4));

    // Baudrate setzen:
    USART2->BRR = SystemCoreClock / 115200;

    // USART2 Transmitter und Resiever aktivieren:
    USART2->CR1 |= USART_CR1_TE;
    USART2->CR1 |= USART_CR1_RE;

    // USART2 einschalten:
    USART2->CR1 |= USART_CR1_UE;
}

void uart_send_byte(uint8_t data) {
    while(!(USART2->ISR & USART_ISR_TXE)) {

    }
    USART2->TDR = data;
}

void uart_send_string(char *text)
{
    while (*text != '\0')
    {
        uart_send_byte(*text);
        text++;
    }
}

uint8_t uart_receive_byte(void) {
    while(!(USART2->ISR & USART_ISR_RXNE)) {

    }
    return USART2->RDR;
}