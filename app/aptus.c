#include "aptus.h"

#include <stdio.h>

static uint32_t aptus_reverse_bits32(uint32_t value) {
    uint32_t reversed = 0U;

    for(uint8_t i = 0U; i < 32U; i++) {
        reversed <<= 1U;
        reversed |= value & 1U;
        value >>= 1U;
    }

    return reversed;
}

uint32_t aptus_serial28_from_fixed32(uint32_t fixed32) {
    return aptus_reverse_bits32(fixed32) & 0x0FFFFFFFU;
}

uint8_t aptus_button_from_fixed32(uint32_t fixed32) {
    const uint8_t button_code = (uint8_t)(aptus_reverse_bits32(fixed32) >> 28U);

    switch(button_code) {
    case 0x2U:
        return 1U;
    case 0x4U:
        return 2U;
    default:
        return 0U;
    }
}

bool aptus_decode_fixed32(uint32_t fixed32, Credential* out) {
    if(!out) return false;

    const uint8_t button = aptus_button_from_fixed32(fixed32);
    if(button == 0U) return false;

    const uint32_t serial28 = aptus_serial28_from_fixed32(fixed32);
    const uint32_t base = serial28 << 1U;
    const uint32_t aptus_id = button == 1U ? (base | 1U) : base;

    char value[CREDENTIAL_VALUE_MAX];
    const int written = snprintf(value, sizeof(value), "%lu", (unsigned long)aptus_id);

    if(written <= 0 || (size_t)written >= sizeof(value)) return false;

    return credential_set(out, CredentialTypeAptusRadio, value, button);
}
