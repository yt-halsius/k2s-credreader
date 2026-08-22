#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LF_READER_DATA_MAX 16U

typedef struct LfReader LfReader;

typedef struct {
    uint32_t sequence;
    bool present;
    uint8_t data[LF_READER_DATA_MAX];
    size_t data_len;
} LfReaderStatus;

LfReader* lf_reader_alloc(void);
bool lf_reader_start(LfReader* reader);
void lf_reader_get_status(LfReader* reader, LfReaderStatus* status);
void lf_reader_free(LfReader* reader);
