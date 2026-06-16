#include "protocol.h"
#include "uart.h"
#include "crc.h"
#include "field.h"

#include <stdint.h>
#include <stdlib.h>

#include "stm32f0xx.h"

#define FRAME_BUFFER_SIZE 32
#define FIELD_SIZE 10

static uint8_t fired_field[FIELD_SIZE][FIELD_SIZE];

static uint8_t last_shot_row = 0;
static uint8_t last_shot_col = 0;

static uint8_t target_active = 0;
static uint8_t target_row = 0;
static uint8_t target_col = 0;
static uint8_t target_direction = 0;

static uint8_t sfr_count = 0;
static uint8_t game_over = 0;
static uint32_t seed_counter = 1;

void protocol_send_message(const char msg_id[3], const uint8_t *payload, uint8_t len) {
    uint8_t crc_data[32];
    uint8_t crc;

    crc_data[0] = msg_id[0];
    crc_data[1] = msg_id[1];
    crc_data[2] = msg_id[2];
    crc_data[3] = len;

    for(uint8_t i = 0; i < len; i++) {
        crc_data[4 + i] = payload[i];
    }

    crc = crc_calculate(crc_data, 4 + len);

    uart_send_byte('#');
    uart_send_byte(msg_id[0]);
    uart_send_byte(msg_id[1]);
    uart_send_byte(msg_id[2]);
    uart_send_byte(len);

    for(uint8_t i = 0; i < len; i++) {
        uart_send_byte(payload[i]);
    }

    uart_send_byte(crc);
    uart_send_byte('$');
}

static void reset_fire_logic(void)
{
    for(uint8_t row = 0; row < FIELD_SIZE; row++)
    {
        for(uint8_t col = 0; col < FIELD_SIZE; col++)
        {
            fired_field[row][col] = 0;
        }
    }

    target_active = 0;
    target_row = 0;
    target_col = 0;
    target_direction = 0;
}

static uint8_t get_target_shot(uint8_t *row, uint8_t *col)
{
    while(target_direction < 4)
    {
        int8_t r = target_row;
        int8_t c = target_col;

        if(target_direction == 0)      // oben
        {
            r--;
        }
        else if(target_direction == 1) // unten
        {
            r++;
        }
        else if(target_direction == 2) // links
        {
            c--;
        }
        else if(target_direction == 3) // rechts
        {
            c++;
        }

        target_direction++;

        if(r >= 0 && r < FIELD_SIZE && c >= 0 && c < FIELD_SIZE)
        {
            if(fired_field[r][c] == 0)
            {
                fired_field[r][c] = 1;

                *row = r;
                *col = c;

                return 1;
            }
        }
    }

    target_active = 0;
    return 0;
}

static void get_next_shot(uint8_t *row, uint8_t *col)
{
    if(target_active)
    {
        if(get_target_shot(row, col))
        {
            return;
        }
    }

    while(1)
    {
        uint8_t r = rand() % FIELD_SIZE;
        uint8_t c = rand() % FIELD_SIZE;

        if(fired_field[r][c] == 0)
        {
            fired_field[r][c] = 1;

            *row = r;
            *col = c;

            return;
        }
    }
}

void protocol_receive_frame(void) {
    uint8_t buffer[FRAME_BUFFER_SIZE];
    uint8_t index = 0;
    uint8_t byte;

    do {
        byte = uart_receive_byte();
    }
    while (byte != '#');

    buffer[index++] = byte;

    while(index < FRAME_BUFFER_SIZE) {
        byte = uart_receive_byte();
        buffer[index++] = byte;

        if(byte == '$') {
            break;
        }
    }

    // MSG-ID steht bei buffer[1], buffer[2], buffer[3]
    if(buffer[1] == 'S' && buffer[2] == 'T' && buffer[3] == 'R')
    {
        srand(seed_counter++); // Für random seed
        field_init();
        reset_fire_logic();

        sfr_count = 0;
        game_over = 0;

        uint8_t name[] = "TEAM01";
        protocol_send_message("STR", name, 6);
    }

    else if(buffer[1] == 'C' && buffer[2] == 'S' && buffer[3] == 'H')
    {
        uint8_t cs[10];

        field_get_checksum(cs);

        protocol_send_message("CSH", cs, 10);
    }

    else if(buffer[1] == 'B' && buffer[2] == 'O' && buffer[3] == 'O')
    {
        uint8_t row = buffer[5];
        uint8_t col = buffer[6];

        uint8_t hit = field_shot_at(row, col);

        if(field_all_ships_hit())
        {
            game_over = 1;

            for(uint8_t row = 0; row < FIELD_SIZE; row++)
            {
                uint8_t payload[11];
                field_get_row(row, payload);
                protocol_send_message("SFR", payload, 11);
            }

            return;
        }

        uint8_t result[] = { hit ? 'H' : 'M' };
        protocol_send_message("BMR", result, 1);

        uint8_t shot_r;
        uint8_t shot_c;

        get_next_shot(&shot_r, &shot_c);

        last_shot_row = shot_r;
        last_shot_col = shot_c;

        uint8_t shot[] = {shot_r, shot_c};
        protocol_send_message("BOO", shot, 2);
    }
    
    else if(buffer[1] == 'B' && buffer[2] == 'M' && buffer[3] == 'R')
    {
        uint8_t hit = (buffer[5] == 'H');

        if(hit)
        {
            target_active = 1;
            target_row = last_shot_row;
            target_col = last_shot_col;
            target_direction = 0;
        }
    }

    else if(buffer[1] == 'S' && buffer[2] == 'F' && buffer[3] == 'R')
    {
        if(game_over == 1)
        {
            return; // SFR nach Spielende nur schlucken, nicht beantworten
        }

        sfr_count++;

        if(sfr_count >= 10)
        {
            sfr_count = 0;

            for(uint8_t row = 0; row < FIELD_SIZE; row++)
            {
                uint8_t payload[11];

                field_get_row(row, payload);

                protocol_send_message("SFR", payload, 11);
            }

            game_over = 1;
        }
    }
}