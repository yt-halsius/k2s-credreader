#include "nfc_reader.h"

#include <furi.h>
#include <nfc/nfc.h>
#include <nfc/protocols/iso14443_3a/iso14443_3a.h>
#include <nfc/protocols/iso14443_3a/iso14443_3a_poller_sync.h>

#include <stdlib.h>
#include <string.h>

#define NFC_READER_POLL_MS 100U

struct NfcReader {
    Nfc* nfc;
    Iso14443_3aData* data;
    FuriThread* thread;
    FuriMutex* mutex;
    NfcReaderStatus status;
    volatile bool running;
};

static int32_t nfc_reader_thread(void* context) {
    NfcReader* reader = context;

    while(reader->running) {
        const Iso14443_3aError error =
            iso14443_3a_poller_sync_read(reader->nfc, reader->data);

        if(error == Iso14443_3aErrorNone) {
            size_t uid_len = 0U;
            const uint8_t* uid = iso14443_3a_get_uid(reader->data, &uid_len);

            if(uid && uid_len <= NFC_READER_UID_MAX) {
                if(furi_mutex_acquire(reader->mutex, FuriWaitForever) == FuriStatusOk) {
                    const bool changed =
                        !reader->status.present ||
                        reader->status.uid_len != uid_len ||
                        memcmp(reader->status.uid, uid, uid_len) != 0;

                    reader->status.present = true;
                    if(changed) {
                        memcpy(reader->status.uid, uid, uid_len);
                        reader->status.uid_len = uid_len;
                        reader->status.sequence++;
                        if(reader->status.sequence == 0U) reader->status.sequence = 1U;
                    }
                    furi_mutex_release(reader->mutex);
                }
            }
        } else {
            if(furi_mutex_acquire(reader->mutex, FuriWaitForever) == FuriStatusOk) {
                reader->status.present = false;
                furi_mutex_release(reader->mutex);
            }
        }

        furi_delay_ms(NFC_READER_POLL_MS);
    }

    return 0;
}

NfcReader* nfc_reader_alloc(void) {
    NfcReader* reader = malloc(sizeof(NfcReader));
    if(!reader) return NULL;
    memset(reader, 0, sizeof(NfcReader));

    reader->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    if(!reader->mutex) {
        free(reader);
        return NULL;
    }

    reader->nfc = nfc_alloc();
    if(!reader->nfc) {
        furi_mutex_free(reader->mutex);
        free(reader);
        return NULL;
    }

    reader->data = iso14443_3a_alloc();
    if(!reader->data) {
        nfc_free(reader->nfc);
        furi_mutex_free(reader->mutex);
        free(reader);
        return NULL;
    }

    reader->thread =
        furi_thread_alloc_ex("K2sNfcReader", 2048U, nfc_reader_thread, reader);
    if(!reader->thread) {
        iso14443_3a_free(reader->data);
        nfc_free(reader->nfc);
        furi_mutex_free(reader->mutex);
        free(reader);
        return NULL;
    }

    return reader;
}

bool nfc_reader_start(NfcReader* reader) {
    if(!reader || !reader->thread || reader->running) return false;

    reader->running = true;
    furi_thread_start(reader->thread);
    return true;
}

void nfc_reader_get_status(NfcReader* reader, NfcReaderStatus* status) {
    if(!reader || !status) return;

    if(furi_mutex_acquire(reader->mutex, FuriWaitForever) == FuriStatusOk) {
        *status = reader->status;
        furi_mutex_release(reader->mutex);
    }
}

void nfc_reader_free(NfcReader* reader) {
    if(!reader) return;

    if(reader->thread) {
        if(reader->running) {
            reader->running = false;
            furi_thread_join(reader->thread);
        }
        furi_thread_free(reader->thread);
    }

    if(reader->data) iso14443_3a_free(reader->data);
    if(reader->nfc) nfc_free(reader->nfc);
    if(reader->mutex) furi_mutex_free(reader->mutex);

    memset(reader, 0, sizeof(NfcReader));
    free(reader);
}
