#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct SubGhzReader SubGhzReader;

typedef struct {
    bool running;
    uint32_t frequency;
    float rssi;
    uint32_t valid_frames;
    uint32_t sequence;
    uint32_t factory_code;
    uint8_t function_code;
} SubGhzReaderStatus;

SubGhzReader* subghz_reader_alloc(void);
void subghz_reader_free(SubGhzReader* reader);
bool subghz_reader_start(SubGhzReader* reader);
void subghz_reader_stop(SubGhzReader* reader);
void subghz_reader_get_status(SubGhzReader* reader, SubGhzReaderStatus* status);
