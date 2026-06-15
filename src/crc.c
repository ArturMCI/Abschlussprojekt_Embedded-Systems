#include "protocol.h"
#include "uart.h"
#include "crc.h"

uint8_t crc_calculate(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0x00;

    for(uint8_t i = 0; i < len; i++)
    {
        crc ^= data[i];

        for(uint8_t bit = 0; bit < 8; bit++)
        {
            if(crc & 0x80)
            {
                crc = (crc << 1) ^ 0x07;
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}

bool crc_check(const uint8_t *data, uint8_t len, uint8_t received_crc) {
    uint8_t calculated_crc;

    calculated_crc = crc_calculate(data, len);

    if(calculated_crc == received_crc) {
        return true;
    }
    return false;
}