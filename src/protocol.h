#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

void protocol_send_message(const char msg_id[3], const uint8_t *payload, uint8_t len);
void protocol_receive_frame(void);

#endif