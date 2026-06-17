#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#include <stdint.h>

void led_matrix_init(void);
void led_matrix_clear(void);
void led_matrix_set(uint8_t row, uint8_t col, uint8_t on);
void led_matrix_show_field(void);

void led_matrix_refresh_row(void);
void led_matrix_refresh_test(void);

void led_matrix_pause(void);
void led_matrix_resume(void);

void led_matrix_blink_hit(uint8_t row, uint8_t col);

#endif