#ifndef FIELD_H
#define FIELD_H

#include <stdint.h>

void field_init(void);

uint8_t field_shot_at(uint8_t row, uint8_t col);

void field_get_row(uint8_t row, uint8_t *payload);

void field_get_checksum(uint8_t *payload);

uint8_t field_all_ships_hit(void);

uint8_t field_has_ship(uint8_t row, uint8_t col);

#endif