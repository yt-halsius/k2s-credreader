#include "lf_reader.h"

#include <furi.h>
#include <lfrfid/lfrfid_worker.h>
#include <lfrfid/protocols/lfrfid_protocols.h>
#include <toolbox/protocols/protocol_dict.h>

#include <stdlib.h>
#include <string.h>

struct LfReader {
    const ProtocolBase** protocol_list;
    ProtocolDict* dict;
    LFRFIDWorker* worker;
    FuriMutex* mutex;
    LfReaderStatus status;
    bool started;
};

static void lf_reader_callback(
    LFRFIDWorkerReadResult result,
    ProtocolId protocol,
    void* context) {
    LfReader* reader = context;
    if(!reader) return;

    /*
     * This reader intentionally contains exactly one protocol: EM4100.
     * Therefore the dictionary-local protocol index is always 0.
     */
    if(result != LFRFIDWorkerReadDone || protocol != 0) return;

    const size_t data_len = protocol_dict_get_data_size(reader->dict, 0);
    if(data_len == 0U || data_len > LF_READER_DATA_MAX) return;

    uint8_t data[LF_READER_DATA_MAX] = {0};
    protocol_dict_get_data(reader->dict, 0, data, sizeof(data));

    if(furi_mutex_acquire(reader->mutex, FuriWaitForever) == FuriStatusOk) {
        const bool changed =
            !reader->status.present ||
            reader->status.data_len != data_len ||
            memcmp(reader->status.data, data, data_len) != 0;

        reader->status.present = true;

        if(changed) {
            memcpy(reader->status.data, data, data_len);
            reader->status.data_len = data_len;
            reader->status.sequence++;
            if(reader->status.sequence == 0U) reader->status.sequence = 1U;
        }

        furi_mutex_release(reader->mutex);
    }

    memset(data, 0, sizeof(data));
}

LfReader* lf_reader_alloc(void) {
    LfReader* reader = malloc(sizeof(LfReader));
    if(!reader) return NULL;
    memset(reader, 0, sizeof(LfReader));

    reader->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    if(!reader->mutex) goto fail;

    /*
     * Key difference from rc1/rc3:
     * do NOT instantiate every LF protocol in Momentum.
     *
     * protocol_dict_alloc() calls alloc() for every protocol supplied to it.
     * K2S only needs EM4100, so export exactly that native decoder into a
     * one-entry dictionary.
     */
    reader->protocol_list = malloc(sizeof(ProtocolBase*));
    if(!reader->protocol_list) goto fail;

    reader->protocol_list[0] = lfrfid_protocols[LFRFIDProtocolEM4100];
    if(!reader->protocol_list[0]) goto fail;

    reader->dict = protocol_dict_alloc(reader->protocol_list, 1U);
    if(!reader->dict) goto fail;

    reader->worker = lfrfid_worker_alloc(reader->dict);
    if(!reader->worker) goto fail;

    return reader;

fail:
    if(reader->worker) lfrfid_worker_free(reader->worker);
    if(reader->dict) protocol_dict_free(reader->dict);
    if(reader->protocol_list) free(reader->protocol_list);
    if(reader->mutex) furi_mutex_free(reader->mutex);
    memset(reader, 0, sizeof(LfReader));
    free(reader);
    return NULL;
}

bool lf_reader_start(LfReader* reader) {
    if(!reader || !reader->worker || reader->started) return false;

    lfrfid_worker_start_thread(reader->worker);
    lfrfid_worker_read_start(
        reader->worker,
        LFRFIDWorkerReadTypeASKOnly,
        lf_reader_callback,
        reader);

    reader->started = true;
    return true;
}

void lf_reader_get_status(LfReader* reader, LfReaderStatus* status) {
    if(!reader || !status) return;

    if(furi_mutex_acquire(reader->mutex, FuriWaitForever) == FuriStatusOk) {
        *status = reader->status;
        furi_mutex_release(reader->mutex);
    }
}

void lf_reader_free(LfReader* reader) {
    if(!reader) return;

    if(reader->worker) {
        if(reader->started) {
            lfrfid_worker_stop(reader->worker);

            /*
             * stop() requests the worker to leave its active read mode.
             * Give the worker time to reach Idle before stop_thread(), whose
             * implementation expects an idle worker.
             */
            furi_delay_ms(100);
            lfrfid_worker_stop_thread(reader->worker);
        }

        lfrfid_worker_free(reader->worker);
    }

    if(reader->dict) protocol_dict_free(reader->dict);
    if(reader->protocol_list) free(reader->protocol_list);
    if(reader->mutex) furi_mutex_free(reader->mutex);

    memset(reader, 0, sizeof(LfReader));
    free(reader);
}
