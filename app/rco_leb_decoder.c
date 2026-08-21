#include "rco_leb_decoder.h"

#include <string.h>

#define RCO_LEB_GAP_MIN_US 2000U
#define RCO_LEB_START_MIN_US 40U
#define RCO_LEB_START_MAX_US 1200U
#define RCO_LEB_PULSE_MIN_US 40U
#define RCO_LEB_PULSE_MAX_US 1200U
#define RCO_LEB_PAIR_MIN_US 120U
#define RCO_LEB_PAIR_MAX_US 1800U
#define RCO_LEB_FRAME_BITS 64U
#define RCO_LEB_HEADER_BITS 40U

static void rco_leb_decoder_reset(RcoLebDecoder* decoder, RcoLebDecoderState state) {
    decoder->state = state;
    decoder->low_duration_us = 0;
    decoder->bit_index = 0;
    decoder->header40 = 0;
}

void rco_leb_decoder_init(RcoLebDecoder* decoder) {
    if(!decoder) return;
    memset(decoder, 0, sizeof(RcoLebDecoder));
    decoder->state = RcoLebDecoderWaitGap;
}

bool rco_leb_decoder_feed(RcoLebDecoder* decoder, bool level, uint32_t duration_us, RcoLebFrame* out_frame) {
    if(!decoder || !out_frame) return false;

    if(!level && duration_us >= RCO_LEB_GAP_MIN_US) {
        rco_leb_decoder_reset(decoder, RcoLebDecoderWaitStart);
        return false;
    }

    switch(decoder->state) {
    case RcoLebDecoderWaitGap:
        return false;

    case RcoLebDecoderWaitStart:
        if(level && duration_us >= RCO_LEB_START_MIN_US && duration_us <= RCO_LEB_START_MAX_US) {
            decoder->state = RcoLebDecoderWaitLow;
        } else {
            rco_leb_decoder_reset(decoder, RcoLebDecoderWaitGap);
        }
        return false;

    case RcoLebDecoderWaitLow:
        if(!level && duration_us >= RCO_LEB_PULSE_MIN_US && duration_us <= RCO_LEB_PULSE_MAX_US) {
            decoder->low_duration_us = duration_us;
            decoder->state = RcoLebDecoderWaitHigh;
        } else {
            rco_leb_decoder_reset(decoder, RcoLebDecoderWaitGap);
        }
        return false;

    case RcoLebDecoderWaitHigh:
        if(!level || duration_us < RCO_LEB_PULSE_MIN_US || duration_us > RCO_LEB_PULSE_MAX_US) {
            rco_leb_decoder_reset(decoder, RcoLebDecoderWaitGap);
            return false;
        }

        {
            const uint32_t pair_duration = decoder->low_duration_us + duration_us;
            if(pair_duration < RCO_LEB_PAIR_MIN_US || pair_duration > RCO_LEB_PAIR_MAX_US) {
                rco_leb_decoder_reset(decoder, RcoLebDecoderWaitGap);
                return false;
            }
        }

        const bool bit = duration_us > decoder->low_duration_us;

        if(decoder->bit_index < RCO_LEB_HEADER_BITS) {
            decoder->header40 = (decoder->header40 << 1) | (bit ? 1ULL : 0ULL);
        }

        decoder->bit_index++;

        if(decoder->bit_index == 8U && (uint8_t)decoder->header40 != 0xFEU) {
            rco_leb_decoder_reset(decoder, RcoLebDecoderWaitGap);
            return false;
        }

        if(decoder->bit_index >= RCO_LEB_FRAME_BITS) {
            const uint8_t prefix = (uint8_t)((decoder->header40 >> 32) & 0xFFU);
            const uint32_t factory_code = (uint32_t)((decoder->header40 >> 8) & 0xFFFFFFU);
            const uint8_t function_code = (uint8_t)(decoder->header40 & 0xFFU);
            const bool valid = prefix == 0xFEU && function_code <= 3U;

            if(valid) {
                out_frame->factory_code = factory_code;
                out_frame->function_code = function_code;
            }

            rco_leb_decoder_reset(decoder, RcoLebDecoderWaitGap);
            return valid;
        }

        decoder->state = RcoLebDecoderWaitLow;
        return false;
    }

    return false;
}
