#include "field.h"
#include <stdlib.h>

#define FIELD_SIZE 10

static uint8_t hit_count = 0;

static uint8_t ship_field[FIELD_SIZE][FIELD_SIZE];
static uint8_t shot_field[FIELD_SIZE][FIELD_SIZE];

static uint8_t can_place_ship(uint8_t row, uint8_t col,
                              uint8_t length, uint8_t vertical)
{
    for(uint8_t i = 0; i < length; i++)
    {
        uint8_t r = row + (vertical ? i : 0);
        uint8_t c = col + (vertical ? 0 : i);

        // Schiff würde außerhalb des Spielfelds liegen
        if(r >= FIELD_SIZE || c >= FIELD_SIZE)
        {
            return 0;
        }

        // Alle Nachbarfelder prüfen
        for(int8_t dr = -1; dr <= 1; dr++)
        {
            for(int8_t dc = -1; dc <= 1; dc++)
            {
                int8_t rr = r + dr;
                int8_t cc = c + dc;

                if(rr >= 0 && rr < FIELD_SIZE && cc >= 0 && cc < FIELD_SIZE)
                {
                    if(ship_field[rr][cc] != 0)
                    {
                        return 0;
                    }
                }
            }
        }
    }

    return 1;
}

static void place_ship(uint8_t row, uint8_t col,
                       uint8_t length, uint8_t vertical)
{
    for(uint8_t i = 0; i < length; i++)
    {
        uint8_t r = row + (vertical ? i : 0);
        uint8_t c = col + (vertical ? 0 : i);

        ship_field[r][c] = length;
    }
}

static uint8_t place_random_ship(uint8_t length)
{
    for(uint16_t tries = 0; tries < 1000; tries++)
    {
        uint8_t vertical = rand() % 2;
        uint8_t row = rand() % FIELD_SIZE;
        uint8_t col = rand() % FIELD_SIZE;

        if(can_place_ship(row, col, length, vertical))
        {
            place_ship(row, col, length, vertical);
            return 1;
        }
    }

    return 0;
}

void field_init(void)
{
    uint8_t success = 0;

    while(!success)
    {
        success = 1;
        hit_count = 0;

        for(uint8_t row = 0; row < FIELD_SIZE; row++)
        {
            for(uint8_t col = 0; col < FIELD_SIZE; col++)
            {
                ship_field[row][col] = 0;
                shot_field[row][col] = 0;
            }
        }

        if(!place_random_ship(5)) success = 0;

        if(!place_random_ship(4)) success = 0;
        if(!place_random_ship(4)) success = 0;

        if(!place_random_ship(3)) success = 0;
        if(!place_random_ship(3)) success = 0;
        if(!place_random_ship(3)) success = 0;

        if(!place_random_ship(2)) success = 0;
        if(!place_random_ship(2)) success = 0;
        if(!place_random_ship(2)) success = 0;
        if(!place_random_ship(2)) success = 0;
    }
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