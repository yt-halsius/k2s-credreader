#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NFC_READER_UID_MAX 10U

typedef struct NfcReader NfcReader;

typedef struct {
    uint32_t sequence;
    bool present;
    uint8_t uid[NFC_READER_UID_MAX];
    size_t uid_len;
} NfcReaderStatus;

NfcReader* nfc_reader_alloc(void);
bool nfc_reader_start(NfcReader* reader);
void nfc_reader_get_status(NfcReader* reader, NfcReaderStatus* status);
void nfc_reader_free(NfcReader* reader);
