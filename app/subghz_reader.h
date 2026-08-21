#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct SubGhzReader SubGhzReader;

typedef enum {
    SubGhzReaderProtocolNone = 0,
    SubGhzReaderProtocolRcoLeb,
    SubGhzReaderProtocolAptusKeeLoq,
} SubGhzReaderProtocol;

typedef struct {
    bool running;
    uint32_t frequency;
    float rssi;

    uint32_t valid_frames;
    uint32_t sequence;
    SubGhzReaderProtocol protocol;

    /* Native receiver diagnostics */
    uint32_t native_keeloq_frames;

    /* RCO/LEB */
    uint32_t factory_code;
    uint8_t function_code;

    /* Aptus/KeeLoq */
    uint32_t fixed32;
} SubGhzReaderStatus;

SubGhzReader* subghz_reader_alloc(void);
void subghz_reader_free(SubGhzReader* reader);

bool subghz_reader_start(SubGhzReader* reader);
void subghz_reader_stop(SubGhzReader* reader);

void subghz_reader_get_status(SubGhzReader* reader, SubGhzReaderStatus* status);
