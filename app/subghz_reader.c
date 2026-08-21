#include "subghz_reader.h"

#include <furi.h>
#include <lib/subghz/devices/devices.h>
#include <lib/subghz/devices/cc1101_int/cc1101_int_interconnect.h>
#include "rco_leb_decoder.h"

#define TAG "K2SSubGhz"
#define K2S_SUBGHZ_FREQUENCY 433920000U

struct SubGhzReader {
    const SubGhzDevice* device;
    volatile bool running;
    volatile uint32_t frequency;
    RcoLebDecoder leb_decoder;
    volatile uint32_t valid_frames;
    volatile uint32_t sequence;
    volatile uint32_t factory_code;
    volatile uint8_t function_code;
};

static void subghz_reader_rx_callback(bool level, uint32_t duration, void* context) {
    SubGhzReader* reader = context;
    RcoLebFrame frame = {0};

    if(rco_leb_decoder_feed(&reader->leb_decoder, level, duration, &frame)) {
        reader->factory_code = frame.factory_code;
        reader->function_code = frame.function_code;
        reader->valid_frames++;
        reader->sequence++;
    }
}

SubGhzReader* subghz_reader_alloc(void) {
    SubGhzReader* reader = malloc(sizeof(SubGhzReader));
    if(!reader) return NULL;

    memset(reader, 0, sizeof(SubGhzReader));
    rco_leb_decoder_init(&reader->leb_decoder);
    subghz_devices_init();

    reader->device = subghz_devices_get_by_name(SUBGHZ_DEVICE_CC1101_INT_NAME);
    if(!reader->device) {
        FURI_LOG_E(TAG, "Internal CC1101 not found");
        subghz_devices_deinit();
        free(reader);
        return NULL;
    }
    return reader;
}

void subghz_reader_free(SubGhzReader* reader) {
    if(!reader) return;
    subghz_reader_stop(reader);
    subghz_devices_deinit();
    memset(reader, 0, sizeof(SubGhzReader));
    free(reader);
}

bool subghz_reader_start(SubGhzReader* reader) {
    if(!reader || !reader->device || reader->running) return false;

    reader->valid_frames = 0;
    reader->sequence = 0;
    reader->factory_code = 0;
    reader->function_code = 0;
    rco_leb_decoder_init(&reader->leb_decoder);

    subghz_devices_reset(reader->device);
    subghz_devices_idle(reader->device);
    subghz_devices_load_preset(reader->device, FuriHalSubGhzPresetOok650Async, NULL);
    reader->frequency = subghz_devices_set_frequency(reader->device, K2S_SUBGHZ_FREQUENCY);
    subghz_devices_flush_rx(reader->device);
    subghz_devices_start_async_rx(reader->device, subghz_reader_rx_callback, reader);
    reader->running = true;

    FURI_LOG_I(TAG, "RX started at %lu Hz", (unsigned long)reader->frequency);
    return true;
}

void subghz_reader_stop(SubGhzReader* reader) {
    if(!reader || !reader->device || !reader->running) return;
    subghz_devices_stop_async_rx(reader->device);
    subghz_devices_idle(reader->device);
    reader->running = false;
}

void subghz_reader_get_status(SubGhzReader* reader, SubGhzReaderStatus* status) {
    if(!reader || !status) return;

    FURI_CRITICAL_ENTER();
    status->running = reader->running;
    status->frequency = reader->frequency;
    status->valid_frames = reader->valid_frames;
    status->sequence = reader->sequence;
    status->factory_code = reader->factory_code;
    status->function_code = reader->function_code;
    FURI_CRITICAL_EXIT();

    status->rssi = status->running ? subghz_devices_get_rssi(reader->device) : -127.0f;
}
