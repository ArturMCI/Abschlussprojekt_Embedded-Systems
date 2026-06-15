#ifndef CRC_H
#define CRC_H

#include <stdint.h>
#include <stdbool.h>

uint8_t crc_calculate(const uint8_t *data, uint8_t len);
bool crc_check(const uint8_t *data, uint8_t len, uint8_t received_crc);

#endif