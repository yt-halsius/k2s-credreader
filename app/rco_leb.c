#include "rco_leb.h"

#include <stdio.h>
#include <string.h>

static bool rco_leb_valid_channel(uint8_t channel) {
    return channel >= 1U && channel <= 4U;
}

static bool rco_leb_parse_binary(
    const char* bits,
    size_t start,
    size_t length,
    uint32_t* value) {
    uint32_t result = 0;

    for(size_t i = 0; i < length; i++) {
        const char c = bits[start + i];
        if(c != '0' && c != '1') return false;

        result <<= 1;
        if(c == '1') result |= 1U;
    }

    *value = result;
    return true;
}

bool rco_leb_decode_factory(
    uint32_t factory_code,
    uint8_t channel,
    RcoLebCredential* out) {
    if(!out || factory_code > 0xFFFFFFU || !rco_leb_valid_channel(channel)) return false;

    const uint8_t byte2 = (factory_code >> 8) & 0xFFU;
    const uint8_t byte3 = factory_code & 0xFFU;
    const uint32_t right = ((uint32_t)byte3 << 8) + (uint32_t)(channel - 1U);

    char combined[16];
    const int written =
        snprintf(combined, sizeof(combined), "%u%lu", byte2, (unsigned long)right);

    if(written <= 0 || (size_t)written >= sizeof(combined)) return false;

    const size_t len = strlen(combined);
    if(len > RCO_ID_LENGTH) return false;

    const size_t pad = RCO_ID_LENGTH - len;
    memset(out->rco_id, '0', pad);
    memcpy(out->rco_id + pad, combined, len);
    out->rco_id[RCO_ID_LENGTH] = '\0';

    out->factory_code = factory_code;
    out->channel = channel;

    return true;
}

bool rco_leb_decode_34bit(
    const char bits[RCO_LEB_BITS + 1],
    uint8_t channel,
    RcoLebCredential* out) {
    if(!bits || !out || strlen(bits) != RCO_LEB_BITS) return false;

    uint32_t prefix = 0;
    uint32_t factory_code = 0;
    uint32_t status = 0;

    if(!rco_leb_parse_binary(bits, 0, 8, &prefix)) return false;
    if(prefix != 0xFEU) return false;

    if(!rco_leb_parse_binary(bits, 8, 24, &factory_code)) return false;

    if(!rco_leb_parse_binary(bits, 32, 2, &status)) return false;
    (void)status;

    return rco_leb_decode_factory(factory_code, channel, out);
}
