#include "protocol.h"
#include "uart.h"
#include "crc.h"
#include "field.h"
#include "led_matrix.h"

#include <stdint.h>
#include <stdlib.h>

#include "stm32f0xx.h"

#define FRAME_BUFFER_SIZE 32
#define FIELD_SIZE 10

static uint8_t fired_field[FIELD_SIZE][FIELD_SIZE];

static uint8_t last_shot_row = 0;
static uint8_t last_shot_col = 0;
static uint8_t shot_pending = 0;

static uint8_t target_active = 0;
static uint8_t target_row = 0;
static uint8_t target_col = 0;
static uint8_t target_direction = 0;

static uint8_t hit_count_strategy = 0;
static uint8_t hit_rows[5];
static uint8_t hit_cols[5];

static uint8_t sfr_count = 0;
static uint8_t game_over = 0;
static uint8_t validation_failed = 0;
static uint32_t seed_counter = 1;

static uint8_t enemy_csh[FIELD_SIZE];
static uint8_t enemy_csh_received = 0;
static uint8_t enemy_field[FIELD_SIZE][FIELD_SIZE];
static uint8_t enemy_sfr_received[FIELD_SIZE];
static uint8_t enemy_sfr_invalid = 0;
static uint8_t shot_result[FIELD_SIZE][FIELD_SIZE];

void protocol_send_message(const char msg_id[3], const uint8_t *payload, uint8_t len)
{
    led_matrix_pause();

    uint8_t crc_data[32];
    uint8_t crc;

    crc_data[0] = msg_id[0];
    crc_data[1] = msg_id[1];
    crc_data[2] = msg_id[2];
    crc_data[3] = len;

    for(uint8_t i = 0; i < len; i++)
    {
        crc_data[4 + i] = payload[i];
    }

    crc = crc_calculate(crc_data, 4 + len);

    uart_send_byte('#');
    uart_send_byte(msg_id[0]);
    uart_send_byte(msg_id[1]);
    uart_send_byte(msg_id[2]);
    uart_send_byte(len);

    for(uint8_t i = 0; i < len; i++)
    {
        uart_send_byte(payload[i]);
    }

    uart_send_byte(crc);
    uart_send_byte('$');

    led_matrix_resume();
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

    hit_count_strategy = 0;
    shot_pending = 0;
}

static void reset_validation_data(void)
{
    enemy_csh_received = 0;
    enemy_sfr_invalid = 0;
    validation_failed = 0;

    for(uint8_t row = 0; row < FIELD_SIZE; row++)
    {
        enemy_csh[row] = 0;
        enemy_sfr_received[row] = 0;

        for(uint8_t col = 0; col < FIELD_SIZE; col++)
        {
            enemy_field[row][col] = 0;
            shot_result[row][col] = 0;
        }
    }
}

static uint8_t validate_enemy_checksum(void)
{
    if(!enemy_csh_received)
    {
        return 0;
    }

    for(uint8_t row = 0; row < FIELD_SIZE; row++)
    {
        uint8_t count = 0;

        for(uint8_t col = 0; col < FIELD_SIZE; col++)
        {
            if(enemy_field[row][col] != 0)
            {
                count++;
            }
        }

        if(enemy_csh[row] != count)
        {
            return 0;
        }
    }

    return 1;
}

static uint8_t validate_shot_protocol(void)
{
    for(uint8_t row = 0; row < FIELD_SIZE; row++)
    {
        for(uint8_t col = 0; col < FIELD_SIZE; col++)
        {
            if(shot_result[row][col] == 1 && enemy_field[row][col] != 0)
            {
                return 0;
            }

            if(shot_result[row][col] == 2 && enemy_field[row][col] == 0)
            {
                return 0;
            }
        }
    }

    return 1;
}

static uint8_t enemy_has_neighbor_outside_ship(uint8_t row, uint8_t col,
                                               uint8_t length, uint8_t vertical)
{
    for(uint8_t i = 0; i < length; i++)
    {
        int8_t r = row + (vertical ? i : 0);
        int8_t c = col + (vertical ? 0 : i);

        for(int8_t dr = -1; dr <= 1; dr++)
        {
            for(int8_t dc = -1; dc <= 1; dc++)
            {
                int8_t rr = r + dr;
                int8_t cc = c + dc;

                if(rr >= 0 && rr < FIELD_SIZE && cc >= 0 && cc < FIELD_SIZE)
                {
                    uint8_t inside_ship = 0;

                    for(uint8_t j = 0; j < length; j++)
                    {
                        uint8_t sr = row + (vertical ? j : 0);
                        uint8_t sc = col + (vertical ? 0 : j);

                        if(rr == sr && cc == sc)
                        {
                            inside_ship = 1;
                        }
                    }

                    if(!inside_ship && enemy_field[rr][cc] != 0)
                    {
                        return 1;
                    }
                }
            }
        }
    }

    return 0;
}

static uint8_t validate_enemy_ship_placement(void)
{
    uint8_t visited[FIELD_SIZE][FIELD_SIZE];
    uint8_t ship_counts[6] = {0, 0, 0, 0, 0, 0};

    for(uint8_t row = 0; row < FIELD_SIZE; row++)
    {
        if(!enemy_sfr_received[row])
        {
            return 0;
        }

        for(uint8_t col = 0; col < FIELD_SIZE; col++)
        {
            visited[row][col] = 0;

            if(enemy_field[row][col] != 0 &&
               (enemy_field[row][col] < 2 || enemy_field[row][col] > 5))
            {
                return 0;
            }
        }
    }

    for(uint8_t row = 0; row < FIELD_SIZE; row++)
    {
        for(uint8_t col = 0; col < FIELD_SIZE; col++)
        {
            uint8_t length = enemy_field[row][col];

            if(length == 0 || visited[row][col])
            {
                continue;
            }

            uint8_t horizontal = 0;
            uint8_t vertical = 0;

            if(col + 1 < FIELD_SIZE && enemy_field[row][col + 1] == length)
            {
                horizontal = 1;
            }

            if(row + 1 < FIELD_SIZE && enemy_field[row + 1][col] == length)
            {
                vertical = 1;
            }

            if(horizontal && vertical)
            {
                return 0;
            }

            if(length > 1 && !horizontal && !vertical)
            {
                return 0;
            }

            for(uint8_t i = 0; i < length; i++)
            {
                uint8_t r = row + (vertical ? i : 0);
                uint8_t c = col + (vertical ? 0 : i);

                if(r >= FIELD_SIZE || c >= FIELD_SIZE ||
                   enemy_field[r][c] != length || visited[r][c])
                {
                    return 0;
                }

                visited[r][c] = 1;
            }

            if(horizontal && col + length < FIELD_SIZE &&
               enemy_field[row][col + length] == length)
            {
                return 0;
            }

            if(vertical && row + length < FIELD_SIZE &&
               enemy_field[row + length][col] == length)
            {
                return 0;
            }

            if(enemy_has_neighbor_outside_ship(row, col, length, vertical))
            {
                return 0;
            }

            ship_counts[length]++;
        }
    }

    return ship_counts[2] == 4 &&
           ship_counts[3] == 3 &&
           ship_counts[4] == 2 &&
           ship_counts[5] == 1;
}

static uint8_t validate_enemy_phase4(void)
{
    return !enemy_sfr_invalid &&
           validate_enemy_checksum() &&
           validate_shot_protocol() &&
           validate_enemy_ship_placement();
}

static void handle_validation_failure(void)
{
    validation_failed = 1;
    game_over = 1;
    sfr_count = 0;
}

static void send_own_sfr_records(void)
{
    for(uint8_t row = 0; row < FIELD_SIZE; row++)
    {
        uint8_t payload[11];
        field_get_row(row, payload);
        protocol_send_message("SFR", payload, 11);
    }
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

static uint8_t get_line_shot(uint8_t *row, uint8_t *col)
{
    if(hit_count_strategy < 2)
    {
        return 0;
    }

    uint8_t horizontal = 1;
    uint8_t vertical = 1;

    for(uint8_t i = 1; i < hit_count_strategy; i++)
    {
        if(hit_rows[i] != hit_rows[0])
        {
            horizontal = 0;
        }

        if(hit_cols[i] != hit_cols[0])
        {
            vertical = 0;
        }
    }

    if(!horizontal && !vertical)
    {
        return 0;
    }

    uint8_t min_row = hit_rows[0];
    uint8_t max_row = hit_rows[0];
    uint8_t min_col = hit_cols[0];
    uint8_t max_col = hit_cols[0];

    for(uint8_t i = 1; i < hit_count_strategy; i++)
    {
        if(hit_rows[i] < min_row) min_row = hit_rows[i];
        if(hit_rows[i] > max_row) max_row = hit_rows[i];

        if(hit_cols[i] < min_col) min_col = hit_cols[i];
        if(hit_cols[i] > max_col) max_col = hit_cols[i];
    }

    if(horizontal)
    {
        uint8_t r = hit_rows[0];

        if(min_col > 0 && fired_field[r][min_col - 1] == 0)
        {
            *row = r;
            *col = min_col - 1;
            fired_field[*row][*col] = 1;
            return 1;
        }

        if(max_col < FIELD_SIZE - 1 && fired_field[r][max_col + 1] == 0)
        {
            *row = r;
            *col = max_col + 1;
            fired_field[*row][*col] = 1;
            return 1;
        }
    }

    if(vertical)
    {
        uint8_t c = hit_cols[0];

        if(min_row > 0 && fired_field[min_row - 1][c] == 0)
        {
            *row = min_row - 1;
            *col = c;
            fired_field[*row][*col] = 1;
            return 1;
        }

        if(max_row < FIELD_SIZE - 1 && fired_field[max_row + 1][c] == 0)
        {
            *row = max_row + 1;
            *col = c;
            fired_field[*row][*col] = 1;
            return 1;
        }
    }

    hit_count_strategy = 0;
    target_active = 0;

    return 0;
}

static void get_next_shot(uint8_t *row, uint8_t *col)
{
    if(get_line_shot(row, col))
    {
        return;
    }

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

    led_matrix_pause();

    buffer[index++] = byte;

    while(index < FRAME_BUFFER_SIZE) {
        byte = uart_receive_byte();
        buffer[index++] = byte;

        if(byte == '$') {
            break;
        }
    }

    led_matrix_resume();

    if(index < 7)
    {
        return;
    }

    uint8_t payload_len = buffer[4];

    if(payload_len > FRAME_BUFFER_SIZE - 7)
    {
        return;
    }

    uint8_t crc_index = 5 + payload_len;
    uint8_t eof_index = crc_index + 1;

    if(eof_index >= index || eof_index >= FRAME_BUFFER_SIZE)
    {
        return;
    }

    if(buffer[eof_index] != '$')
    {
        return;
    }

    if(!crc_check(&buffer[1], 4 + payload_len, buffer[crc_index]))
    {
        return;
    }

    // MSG-ID steht bei buffer[1], buffer[2], buffer[3]
    if(buffer[1] == 'S' && buffer[2] == 'T' && buffer[3] == 'R')
    {
        srand(seed_counter++); // Für random seed
        field_init();
        reset_fire_logic();
        reset_validation_data();

        sfr_count = 0;
        game_over = 0;

        uint8_t name[] = "TEAM01";
        protocol_send_message("STR", name, 6);
    }

    else if(buffer[1] == 'C' && buffer[2] == 'S' && buffer[3] == 'H')
    {
        if(payload_len != 10)
        {
            return;
        }

        for(uint8_t row = 0; row < FIELD_SIZE; row++)
        {
            if(buffer[5 + row] < '0' || buffer[5 + row] > '9')
            {
                handle_validation_failure();
                return;
            }

            enemy_csh[row] = buffer[5 + row] - '0';
        }

        enemy_csh_received = 1;

        uint8_t cs[10];

        field_get_checksum(cs);

        protocol_send_message("CSH", cs, 10);

        led_matrix_show_field();
    }

    else if(buffer[1] == 'B' && buffer[2] == 'O' && buffer[3] == 'O')
    {
        if(payload_len != 2)
        {
            return;
        }

        uint8_t row = buffer[5];
        uint8_t col = buffer[6];

        uint8_t hit = field_shot_at(row, col);

        if(field_all_ships_hit())
        {
            game_over = 1;
            send_own_sfr_records();

            return;
        }

        uint8_t result[] = { hit ? 'H' : 'M' };
        protocol_send_message("BMR", result, 1);

        uint8_t shot_r;
        uint8_t shot_c;

        get_next_shot(&shot_r, &shot_c);

        last_shot_row = shot_r;
        last_shot_col = shot_c;
        shot_pending = 1;

        uint8_t shot[] = {shot_r, shot_c};
        protocol_send_message("BOO", shot, 2);

        if(hit)
        {
            led_matrix_blink_hit(row, col);
            led_matrix_set(row, col, 0);
        }
    }
    
    else if(buffer[1] == 'B' && buffer[2] == 'M' && buffer[3] == 'R')
    {
        if(payload_len != 1 || (buffer[5] != 'H' && buffer[5] != 'M'))
        {
            return;
        }

        if(!shot_pending)
        {
            return;
        }

        uint8_t hit = (buffer[5] == 'H');
        shot_result[last_shot_row][last_shot_col] = hit ? 2 : 1;
        shot_pending = 0;

        if(hit)
        {
            if(hit_count_strategy < 5)
            {
                hit_rows[hit_count_strategy] = last_shot_row;
                hit_cols[hit_count_strategy] = last_shot_col;
                hit_count_strategy++;
            }

            target_active = 1;
            target_row = last_shot_row;
            target_col = last_shot_col;
            target_direction = 0;
        }
    }

    else if(buffer[1] == 'S' && buffer[2] == 'F' && buffer[3] == 'R')
    {
        if(payload_len != 11)
        {
            return;
        }

        if(!game_over && shot_pending)
        {
            shot_result[last_shot_row][last_shot_col] = 2;
            shot_pending = 0;
        }

        uint8_t received_row = buffer[5];
        uint8_t sfr_row_ok = 1;

        if(received_row >= FIELD_SIZE || enemy_sfr_received[received_row])
        {
            sfr_row_ok = 0;
            enemy_sfr_invalid = 1;
        }

        for(uint8_t col = 0; col < FIELD_SIZE; col++)
        {
            uint8_t cell = buffer[6 + col];

            if(cell != '0' && cell != '2' && cell != '3' &&
               cell != '4' && cell != '5')
            {
                sfr_row_ok = 0;
                enemy_sfr_invalid = 1;
            }

            if(sfr_row_ok)
            {
                enemy_field[received_row][col] = cell - '0';
            }
        }

        if(sfr_row_ok)
        {
            enemy_sfr_received[received_row] = 1;
        }

        sfr_count++;

        uint8_t sfr_exchange_complete = (sfr_count >= FIELD_SIZE ||
                                         received_row == FIELD_SIZE - 1);

        if(game_over == 1)
        {
            if(sfr_exchange_complete)
            {
                if(validate_enemy_phase4())
                {
                    sfr_count = 0;
                    game_over = 0;
                }
                else
                {
                    handle_validation_failure();
                }
            }

            return;
        }

        if(sfr_exchange_complete)
        {
            uint8_t phase4_ok = validate_enemy_phase4();
            send_own_sfr_records();

            sfr_count = 0;

            if(phase4_ok)
            {
                game_over = 1;
            }
            else
            {
                handle_validation_failure();
            }
        }
    }
}
