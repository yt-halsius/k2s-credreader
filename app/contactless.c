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

bool contactless_decode_mifare_uid_order(
    const uint8_t* uid,
    size_t uid_len,
    bool lsb_first,
    Credential* out) {
    if(!uid || !out || uid_len != K2S_MIFARE_UID_LEN) return false;

    char value[11] = {0};
    size_t offset = 0U;
    static const char hex[] = "0123456789ABCDEF";

    value[offset++] = '0';
    value[offset++] = '0';

    for(size_t i = 0; i < K2S_MIFARE_UID_LEN; ++i) {
        const size_t index =
            lsb_first ? (K2S_MIFARE_UID_LEN - 1U - i) : i;
        const uint8_t byte = uid[index];
        value[offset++] = hex[(byte >> 4) & 0x0FU];
        value[offset++] = hex[byte & 0x0FU];
    }

    value[offset] = '\0';
    return credential_set(out, CredentialTypeMifare, value, 0U);
}

bool contactless_decode_mifare_uid(
    const uint8_t* uid,
    size_t uid_len,
    Credential* out) {
    return contactless_decode_mifare_uid_order(uid, uid_len, false, out);
}
