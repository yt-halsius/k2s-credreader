#include "contactless.h"

#include <stdio.h>

bool contactless_decode_em4100(
    const uint8_t data[K2S_EM4100_DATA_LEN],
    Credential* out) {
    if(!data || !out) return false;

    char value[13];
    const int written = snprintf(
        value,
        sizeof(value),
        "00%02X%02X%02X%02X%02X",
        data[0],
        data[1],
        data[2],
        data[3],
        data[4]);

    if(written != 12) return false;
    return credential_set(out, CredentialTypeEm, value, 0U);
}

bool contactless_decode_mifare_uid(
    const uint8_t* uid,
    size_t uid_len,
    Credential* out) {
    if(!uid || !out || uid_len != K2S_MIFARE_UID_LEN) return false;

    char value[11];
    const int written = snprintf(
        value,
        sizeof(value),
        "00%02X%02X%02X%02X",
        uid[0],
        uid[1],
        uid[2],
        uid[3]);

    if(written != 10) return false;
    return credential_set(out, CredentialTypeMifare, value, 0U);
}
