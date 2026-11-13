#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "credential.h"

#define K2S_EM4100_DATA_LEN 5U
#define K2S_MIFARE_UID_LEN 4U

bool contactless_decode_em4100(
    const uint8_t data[K2S_EM4100_DATA_LEN],
    Credential* out);

bool contactless_decode_mifare_uid_order(
    const uint8_t* uid,
    size_t uid_len,
    bool lsb_first,
    Credential* out);

bool contactless_decode_mifare_uid(
    const uint8_t* uid,
    size_t uid_len,
    Credential* out);
