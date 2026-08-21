#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "credential.h"

/*
 * The tested Aptus transmitters use the fixed 32-bit half of a 64-bit KeeLoq
 * frame as:
 *
 *   reverse_bits_32(fixed32) = button_code(4) | serial28
 *
 * Observed Aptus button codes:
 *   0x2 -> physical button 1
 *   0x4 -> physical button 2
 *
 * The normalized Aptus credential is:
 *   base = serial28 << 1
 *   button 1 -> base | 1
 *   button 2 -> base
 */
uint32_t aptus_serial28_from_fixed32(uint32_t fixed32);
uint8_t aptus_button_from_fixed32(uint32_t fixed32);

bool aptus_decode_fixed32(uint32_t fixed32, Credential* out);
