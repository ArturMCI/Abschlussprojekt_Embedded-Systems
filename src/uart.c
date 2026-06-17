#include "stm32f0xx.h"

#include "uart.h"

#define UART_RX_BUFFER_SIZE 256

static volatile uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint8_t rx_head = 0;
static volatile uint8_t rx_tail = 0;

static void uart_rx_store(uint8_t data)
{
    uint8_t next_head = (rx_head + 1) % UART_RX_BUFFER_SIZE;

    if(next_head == rx_tail)
    {
        rx_tail = (rx_tail + 1) % UART_RX_BUFFER_SIZE;
    }

    rx_buffer[rx_head] = data;
    rx_head = next_head;
}

void uart_init(void) {
    rx_head = 0;
    rx_tail = 0;

    // Clocks für GPIOA und USART2 aktivieren:
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    //PA2 und PA3 konfigurieren:
    GPIOA->MODER &= ~((0b0011 << 2 * 2) | (0b0011 << 3 * 2));
    GPIOA->MODER |= ((0b0010 << 2 * 2) | (0b0010 << 3 * 2));

    GPIOA->AFR[0] &= ~((0xFU << (2 * 4)) | (0xFU << (3 * 4)));
    GPIOA->AFR[0] |=  ((1U << (2 * 4)) | (1U << (3 * 4)));

    // Baudrate setzen:
    USART2->BRR = SystemCoreClock / 115200;

    // USART2 Transmitter und Resiever aktivieren:
    USART2->CR1 |= USART_CR1_TE;
    USART2->CR1 |= USART_CR1_RE;
    USART2->CR1 |= USART_CR1_RXNEIE;

    NVIC_SetPriority(USART2_IRQn, 0);
    NVIC_EnableIRQ(USART2_IRQn);

    // USART2 einschalten:
    USART2->CR1 |= USART_CR1_UE;
}

void USART2_IRQHandler(void)
{
    if(USART2->ISR & USART_ISR_ORE)
    {
        USART2->ICR = USART_ICR_ORECF;
    }

    if(USART2->ISR & USART_ISR_RXNE)
    {
        uart_rx_store((uint8_t)USART2->RDR);
    }
}

void uart_send_byte(uint8_t data)
{
    while(!(USART2->ISR & USART_ISR_TXE))
    {
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

uint8_t uart_receive_byte(void)
{
    while(1)
    {
        if(rx_tail != rx_head)
        {
            uint8_t data = rx_buffer[rx_tail];
            rx_tail = (rx_tail + 1) % UART_RX_BUFFER_SIZE;
            return data;
        }
    }
}
