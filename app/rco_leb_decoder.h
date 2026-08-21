#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t factory_code;
    uint8_t function_code;
} RcoLebFrame;

typedef enum {
    RcoLebDecoderWaitGap = 0,
    RcoLebDecoderWaitStart,
    RcoLebDecoderWaitLow,
    RcoLebDecoderWaitHigh,
} RcoLebDecoderState;

typedef struct {
    RcoLebDecoderState state;
    uint32_t low_duration_us;
    uint8_t bit_index;
    uint64_t header40;
} RcoLebDecoder;

void rco_leb_decoder_init(RcoLebDecoder* decoder);
bool rco_leb_decoder_feed(RcoLebDecoder* decoder, bool level, uint32_t duration_us, RcoLebFrame* out_frame);
