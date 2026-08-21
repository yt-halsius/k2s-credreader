#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RCO_LEB_BITS 34U
#define RCO_ID_LENGTH 9U

typedef struct {
    uint32_t factory_code;
    uint8_t channel;
    char rco_id[RCO_ID_LENGTH + 1];
} RcoLebCredential;

bool rco_leb_decode_34bit(
    const char* bits,
    uint8_t channel,
    RcoLebCredential* out);

bool rco_leb_decode_factory(
    uint32_t factory_code,
    uint8_t channel,
    RcoLebCredential* out);
