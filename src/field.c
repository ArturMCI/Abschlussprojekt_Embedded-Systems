#include "field.h"

#define FIELD_SIZE 10

static uint8_t hit_count = 0;

static uint8_t ship_field[FIELD_SIZE][FIELD_SIZE];
static uint8_t shot_field[FIELD_SIZE][FIELD_SIZE];

void field_init(void)
{
    hit_count = 0;

    for(uint8_t row = 0; row < FIELD_SIZE; row++)
    {
        for(uint8_t col = 0; col < FIELD_SIZE; col++)
        {
            ship_field[row][col] = 0;
            shot_field[row][col] = 0;
        }
    }
    // 1x Länge 5
    ship_field[0][0] = 5;
    ship_field[0][1] = 5;
    ship_field[0][2] = 5;
    ship_field[0][3] = 5;
    ship_field[0][4] = 5;

    // 2x Länge 4
    ship_field[0][6] = 4;
    ship_field[0][7] = 4;
    ship_field[0][8] = 4;
    ship_field[0][9] = 4;

    ship_field[2][0] = 4;
    ship_field[2][1] = 4;
    ship_field[2][2] = 4;
    ship_field[2][3] = 4;

    // 3x Länge 3
    ship_field[2][5] = 3;
    ship_field[2][6] = 3;
    ship_field[2][7] = 3;

    ship_field[2][9] = 3;
    ship_field[3][9] = 3;
    ship_field[4][9] = 3;

    ship_field[4][0] = 3;
    ship_field[4][1] = 3;
    ship_field[4][2] = 3;

    // 4x Länge 2
    ship_field[4][4] = 2;
    ship_field[4][5] = 2;

    ship_field[4][7] = 2;
    ship_field[5][7] = 2;

    ship_field[6][0] = 2;
    ship_field[6][1] = 2;

    ship_field[6][3] = 2;
    ship_field[6][4] = 2;
}

uint8_t field_shot_at(uint8_t row, uint8_t col)
{
    if(row >= FIELD_SIZE || col >= FIELD_SIZE)
    {
        return 0;
    }

    // Check auf doppelte Schüsse
    if(shot_field[row][col] == 1)
    {
        return 0;
    }

    shot_field[row][col] = 1;

    if(ship_field[row][col] >= 2 && ship_field[row][col] <= 5)
    {
        hit_count++;
        return 1;
    }

    return 0;
}

void field_get_row(uint8_t row, uint8_t *payload)
{
    payload[0] = row;

    for(uint8_t col = 0; col < FIELD_SIZE; col++)
    {
        payload[col + 1] = ship_field[row][col] + '0';
    }
}

void field_get_checksum(uint8_t *payload)
{
    for(uint8_t row = 0; row < FIELD_SIZE; row++)
    {
        uint8_t count = 0;

        for(uint8_t col = 0; col < FIELD_SIZE; col++)
        {
            if(ship_field[row][col] != 0)
            {
                count++;
            }
        }

        payload[row] = count + '0';
    }
}

uint8_t field_all_ships_hit(void)
{
    return hit_count >= 30;
}