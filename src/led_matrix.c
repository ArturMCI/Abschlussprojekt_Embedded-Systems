#include "led_matrix.h"
#include "field.h"
#include "stm32f0xx.h"

#define MATRIX_SIZE 10

#define PIN_RESET     12  // PC12 = Reset
#define PIN_DATA_ALP  5   // PC5  = DATA_ALP
#define PIN_STCP      8   // PC8  = STCP / Latch
#define PIN_CLOCK     4   // PA4  = Clock
#define PIN_DATA_NUM  10  // PB10 = DATA_NUM

static volatile uint8_t led_state[MATRIX_SIZE][MATRIX_SIZE];
static volatile uint8_t scan_row = 0;

static void pin_high(GPIO_TypeDef *port, uint8_t pin)
{
    port->BSRR = (1U << pin);
}

static void pin_low(GPIO_TypeDef *port, uint8_t pin)
{
    port->BRR = (1U << pin);
}

static void delay_short(void)
{
}

static void pulse_clock(void)
{
    pin_high(GPIOA, PIN_CLOCK);
    delay_short();
    pin_low(GPIOA, PIN_CLOCK);
}

static void pulse_latch(void)
{
    pin_high(GPIOC, PIN_STCP);
    delay_short();
    pin_low(GPIOC, PIN_STCP);
}

static void shift_out_16(uint16_t alpha, uint16_t num)
{
    for(int8_t i = 15; i >= 0; i--)
    {
        if(alpha & (1U << i))
            pin_high(GPIOC, PIN_DATA_ALP);
        else
            pin_low(GPIOC, PIN_DATA_ALP);

        if(num & (1U << i))
            pin_high(GPIOB, PIN_DATA_NUM);
        else
            pin_low(GPIOB, PIN_DATA_NUM);

        pulse_clock();
    }

    pulse_latch();
}

void led_matrix_refresh_row(void)
{
    uint16_t alpha = 0x03FF;   // alle 10 Zeilen AUS
    uint16_t num   = 0x0000;   // alle 10 Spalten AUS

    // Nur aktuelle Zeile aktivieren
    alpha &= ~(1U << scan_row);

    for(uint8_t col = 0; col < MATRIX_SIZE; col++)
    {
        if(led_state[scan_row][col])
        {
            num |= (1U << col);   // Schiffsteil = LED AN
        }
    }

    shift_out_16(alpha, num);

    scan_row++;
    if(scan_row >= MATRIX_SIZE)
    {
        scan_row = 0;
    }
}

static void led_matrix_timer_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

    TIM6->CR1 = 0;
    TIM6->PSC = 47;    // 48 MHz / 48 = 1 MHz
    TIM6->ARR = 399;   // 0,4 MHz / 1000 = 2,5 kHz Interrupt

    TIM6->DIER |= TIM_DIER_UIE;
    TIM6->EGR |= TIM_EGR_UG;

    NVIC_SetPriority(TIM6_DAC_IRQn, 2);
    NVIC_EnableIRQ(TIM6_DAC_IRQn);

    TIM6->CR1 |= TIM_CR1_CEN;
}

void TIM6_DAC_IRQHandler(void)
{
    if(TIM6->SR & TIM_SR_UIF)
    {
        TIM6->SR &= ~TIM_SR_UIF;
        led_matrix_refresh_row();
    }
}

void led_matrix_init(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

    GPIOA->MODER &= ~(3U << (PIN_CLOCK * 2));
    GPIOA->MODER |=  (1U << (PIN_CLOCK * 2));

    GPIOB->MODER &= ~(3U << (PIN_DATA_NUM * 2));
    GPIOB->MODER |=  (1U << (PIN_DATA_NUM * 2));

    GPIOC->MODER &= ~(3U << (PIN_RESET * 2));
    GPIOC->MODER |=  (1U << (PIN_RESET * 2));

    GPIOC->MODER &= ~(3U << (PIN_DATA_ALP * 2));
    GPIOC->MODER |=  (1U << (PIN_DATA_ALP * 2));

    GPIOC->MODER &= ~(3U << (PIN_STCP * 2));
    GPIOC->MODER |=  (1U << (PIN_STCP * 2));

    pin_low(GPIOA, PIN_CLOCK);
    pin_low(GPIOB, PIN_DATA_NUM);
    pin_low(GPIOC, PIN_DATA_ALP);
    pin_low(GPIOC, PIN_STCP);

    pin_low(GPIOC, PIN_RESET);
    delay_short();
    pin_high(GPIOC, PIN_RESET);

    led_matrix_clear();
    led_matrix_timer_init();
}

void led_matrix_clear(void)
{
    for(uint8_t row = 0; row < MATRIX_SIZE; row++)
    {
        for(uint8_t col = 0; col < MATRIX_SIZE; col++)
        {
            led_state[row][col] = 0;
        }
    }
}

void led_matrix_set(uint8_t row, uint8_t col, uint8_t on)
{
    if(row >= MATRIX_SIZE || col >= MATRIX_SIZE)
    {
        return;
    }

    uint8_t hw_row = 9 - col;
    uint8_t hw_col = 9 - row;

    led_state[hw_row][hw_col] = on ? 1 : 0;
}

void led_matrix_show_field(void)
{
    led_matrix_clear();

    for(uint8_t row = 0; row < MATRIX_SIZE; row++)
    {
        for(uint8_t col = 0; col < MATRIX_SIZE; col++)
        {
            if(field_has_ship(row, col))
            {
                led_matrix_set(row, col, 1);
            }
        }
    }
}

void led_matrix_pause(void)
{
    TIM6->CR1 &= ~TIM_CR1_CEN;
}

void led_matrix_resume(void)
{
    TIM6->CNT = 0;
    TIM6->CR1 |= TIM_CR1_CEN;
}

static void led_wait(void)
{
    for(volatile uint32_t i = 0; i < 50000; i++);
}

void led_matrix_blink_hit(uint8_t row, uint8_t col)
{
    led_matrix_set(row, col, 0);
    led_wait();
}
