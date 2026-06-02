#include "stm32f0xx.h";

#include uart.h;

void uart_init(void) {
    // Clocks für GPIOA und USART2 aktivieren:
    RCC->AHBNER |= RCC_AHBNER_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    //PA2 und PA3 konfigurieren:
    GPIOA->MODER &= ~(2 << 2 * 2);
    GPIOA->MODER |= (0b0010 << 2 * 2);

    GPIOA->MODER &= ~(3 << 3 * 2);
    GPIOA->MODER |= (0b0010 << 3 * 2);



    // Baudrate setzen:
    USART2->BRR = SystemCoreClock / 115200;

    // USART2 Transmitter und Resiever aktivieren:
    USART2->CR1 |= USART_CR1_TE;
    USART2->CR1 |= USART_CR1_RE;

    // USART2 einschalten:
    USART2->CR1 |= USART_CR1_UE;
}