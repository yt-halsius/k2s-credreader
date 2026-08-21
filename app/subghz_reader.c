#include "subghz_reader.h"

#include <furi.h>
#include <string.h>
#include <flipper_format/flipper_format.h>

#include <lib/subghz/devices/devices.h>
#include <lib/subghz/devices/cc1101_int/cc1101_int_interconnect.h>
#include <lib/subghz/environment.h>
#include <lib/subghz/receiver.h>
#include <lib/subghz/subghz_protocol_registry.h>
#include <lib/subghz/subghz_worker.h>
#include <lib/subghz/protocols/base.h>
#include <lib/subghz/protocols/keeloq.h>

#include "rco_leb_decoder.h"

#define TAG "K2SSubGhz"
#define K2S_SUBGHZ_FREQUENCY 433920000U

struct SubGhzReader {
    const SubGhzDevice* device;
    volatile bool running;
    volatile uint32_t frequency;

    /*
     * Momentum's normal Sub-GHz receive path:
     *
     * CC1101 ISR -> SubGhzWorker -> our pair dispatcher
     *                          -> SubGhzReceiver -> protocol registry
     *
     * The pair dispatcher also feeds the custom RCO/LEB decoder.
     */
    SubGhzWorker* worker;
    SubGhzEnvironment* environment;
    SubGhzReceiver* receiver;

    RcoLebDecoder leb_decoder;

    FlipperFormat* keeloq_format;
    SubGhzRadioPreset keeloq_preset;

    volatile uint32_t valid_frames;
    volatile uint32_t sequence;
    volatile SubGhzReaderProtocol protocol;
    volatile uint32_t native_keeloq_frames;

    volatile uint32_t factory_code;
    volatile uint8_t function_code;
    volatile uint32_t fixed32;
};

static void subghz_reader_publish_aptus(SubGhzReader* reader, uint32_t fixed32) {
    reader->fixed32 = fixed32;
    reader->factory_code = 0U;
    reader->function_code = 0U;
    reader->protocol = SubGhzReaderProtocolAptusKeeLoq;
    reader->native_keeloq_frames++;
    reader->valid_frames++;
    reader->sequence++;
}

static void subghz_reader_receiver_callback(
    SubGhzReceiver* receiver,
    SubGhzProtocolDecoderBase* decoder_base,
    void* context) {
    UNUSED(receiver);

    SubGhzReader* reader = context;
    uint32_t bit_count = 0U;
    uint8_t key[8] = {0};

    if(!reader || !decoder_base || !decoder_base->protocol ||
       !reader->keeloq_format) {
        return;
    }

    /*
     * subghz_receiver_set_rx_callback() is part of the exported FAP API.
     * Momentum's receiver installs the otherwise-private per-decoder
     * callbacks internally and forwards completed protocol frames here.
     */
    if(strcmp(decoder_base->protocol->name, SUBGHZ_PROTOCOL_KEELOQ_NAME) != 0) {
        return;
    }

    if(subghz_protocol_decoder_base_serialize(
           decoder_base,
           reader->keeloq_format,
           &reader->keeloq_preset) != SubGhzProtocolStatusOk) {
        return;
    }

    if(!flipper_format_rewind(reader->keeloq_format)) return;

    if(!flipper_format_read_uint32(
           reader->keeloq_format, "Bit", &bit_count, 1U)) {
        return;
    }

    if(bit_count != 64U) return;

    if(!flipper_format_read_hex(
           reader->keeloq_format, "Key", key, sizeof(key))) {
        return;
    }

    const uint32_t fixed32 =
        ((uint32_t)key[4] << 24U) |
        ((uint32_t)key[5] << 16U) |
        ((uint32_t)key[6] << 8U) |
        (uint32_t)key[7];

    memset(key, 0, sizeof(key));

    /*
     * API 87.1 has no flipper_format_reset(). Recreate the transient
     * in-memory buffer so the serialized 64-bit frame is discarded.
     */
    flipper_format_free(reader->keeloq_format);
    reader->keeloq_format = flipper_format_string_alloc();

    if(!reader->keeloq_format) {
        FURI_LOG_E(TAG, "FlipperFormat re-allocation failed");
        return;
    }

    subghz_reader_publish_aptus(reader, fixed32);
}

static void subghz_reader_pair_callback(
    void* context,
    bool level,
    uint32_t duration) {
    SubGhzReader* reader = context;

    /*
     * 1) Feed the complete native Momentum protocol registry.
     *    KeeLoq is one of the registered Decodable protocols.
     */
    subghz_receiver_decode(reader->receiver, level, duration);

    /*
     * 2) Feed our custom RCO/LEB decoder from the same worker-normalized
     *    level/duration stream.
     */
    RcoLebFrame leb_frame = {0};
    if(rco_leb_decoder_feed(
           &reader->leb_decoder, level, duration, &leb_frame)) {
        reader->factory_code = leb_frame.factory_code;
        reader->function_code = leb_frame.function_code;
        reader->fixed32 = 0U;
        reader->protocol = SubGhzReaderProtocolRcoLeb;
        reader->valid_frames++;
        reader->sequence++;
    }
}

static void subghz_reader_overrun_callback(void* context) {
    SubGhzReader* reader = context;

    subghz_receiver_reset(reader->receiver);
    rco_leb_decoder_init(&reader->leb_decoder);
}

SubGhzReader* subghz_reader_alloc(void) {
    SubGhzReader* reader = malloc(sizeof(SubGhzReader));
    if(!reader) return NULL;

    memset(reader, 0, sizeof(SubGhzReader));
    rco_leb_decoder_init(&reader->leb_decoder);

    subghz_devices_init();

    reader->device =
        subghz_devices_get_by_name(SUBGHZ_DEVICE_CC1101_INT_NAME);
    if(!reader->device) {
        FURI_LOG_E(TAG, "Internal CC1101 not found");
        subghz_devices_deinit();
        free(reader);
        return NULL;
    }

    reader->worker = subghz_worker_alloc();
    if(!reader->worker) {
        FURI_LOG_E(TAG, "SubGhz worker alloc failed");
        subghz_devices_deinit();
        free(reader);
        return NULL;
    }

    reader->environment = subghz_environment_alloc();
    if(!reader->environment) {
        FURI_LOG_E(TAG, "SubGhz environment alloc failed");
        subghz_worker_free(reader->worker);
        subghz_devices_deinit();
        free(reader);
        return NULL;
    }

    /*
     * This is the missing part in v0.5.1: the native receiver must be
     * initialized from the protocol registry, just as Momentum's own
     * Sub-GHz application does.
     */
    subghz_environment_set_protocol_registry(
        reader->environment, (void*)&subghz_protocol_registry);

    reader->receiver = subghz_receiver_alloc_init(reader->environment);
    if(!reader->receiver) {
        FURI_LOG_E(TAG, "SubGhz receiver alloc failed");
        subghz_environment_free(reader->environment);
        subghz_worker_free(reader->worker);
        subghz_devices_deinit();
        free(reader);
        return NULL;
    }

    /*
     * Only decodable protocols need to run. KeeLoq carries this flag.
     */
    subghz_receiver_set_filter(
        reader->receiver, SubGhzProtocolFlag_Decodable);

    /*
     * FAP-safe workaround:
     *
     * the private per-decoder callback setter is disabled in
     * Momentum's external-app API, but subghz_receiver_set_rx_callback() is
     * exported. The receiver performs the private per-decoder hookup inside
     * firmware and forwards completed frames to our callback.
     */
    subghz_receiver_set_rx_callback(
        reader->receiver,
        subghz_reader_receiver_callback,
        reader);

    reader->keeloq_format = flipper_format_string_alloc();
    if(!reader->keeloq_format) {
        FURI_LOG_E(TAG, "FlipperFormat alloc failed");
        subghz_receiver_free(reader->receiver);
        subghz_environment_free(reader->environment);
        subghz_worker_free(reader->worker);
        subghz_devices_deinit();
        free(reader);
        return NULL;
    }

    memset(&reader->keeloq_preset, 0, sizeof(reader->keeloq_preset));
    reader->keeloq_preset.frequency = K2S_SUBGHZ_FREQUENCY;
    reader->keeloq_preset.name = furi_string_alloc_set("AM650");

    /*
     * Match Momentum's own receive architecture:
     * CC1101 ISR -> worker buffer -> pair callback on worker thread.
     */
    subghz_worker_set_overrun_callback(
        reader->worker, subghz_reader_overrun_callback);
    subghz_worker_set_pair_callback(
        reader->worker, subghz_reader_pair_callback);
    subghz_worker_set_context(reader->worker, reader);

    return reader;
}

void subghz_reader_free(SubGhzReader* reader) {
    if(!reader) return;

    subghz_reader_stop(reader);

    if(reader->keeloq_preset.name) {
        furi_string_free(reader->keeloq_preset.name);
        reader->keeloq_preset.name = NULL;
    }

    if(reader->keeloq_format) {
        flipper_format_free(reader->keeloq_format);
        reader->keeloq_format = NULL;
    }

    if(reader->receiver) {
        subghz_receiver_free(reader->receiver);
        reader->receiver = NULL;
    }

    if(reader->environment) {
        subghz_environment_free(reader->environment);
        reader->environment = NULL;
    }

    if(reader->worker) {
        subghz_worker_free(reader->worker);
        reader->worker = NULL;
    }

    subghz_devices_deinit();

    memset(reader, 0, sizeof(SubGhzReader));
    free(reader);
}

bool subghz_reader_start(SubGhzReader* reader) {
    if(!reader || !reader->device || !reader->worker ||
       !reader->receiver || reader->running) {
        return false;
    }

    reader->valid_frames = 0U;
    reader->sequence = 0U;
    reader->protocol = SubGhzReaderProtocolNone;
    reader->native_keeloq_frames = 0U;
    reader->factory_code = 0U;
    reader->function_code = 0U;
    reader->fixed32 = 0U;

    rco_leb_decoder_init(&reader->leb_decoder);
    subghz_receiver_reset(reader->receiver);

    /*
     * Start with a clean transient in-memory serialization buffer.
     * API 87.1 does not expose flipper_format_reset().
     */
    if(reader->keeloq_format) {
        flipper_format_free(reader->keeloq_format);
    }
    reader->keeloq_format = flipper_format_string_alloc();
    if(!reader->keeloq_format) {
        FURI_LOG_E(TAG, "FlipperFormat re-allocation failed");
        return false;
    }

    subghz_devices_reset(reader->device);
    subghz_devices_idle(reader->device);
    subghz_devices_load_preset(
        reader->device, FuriHalSubGhzPresetOok650Async, NULL);

    reader->frequency =
        subghz_devices_set_frequency(reader->device, K2S_SUBGHZ_FREQUENCY);
    subghz_devices_flush_rx(reader->device);

    /*
     * Important difference from v0.5.1:
     * CC1101 no longer calls any decoder directly from the async ISR.
     * It feeds Momentum's SubGhzWorker exactly like the stock receive path.
     */
    subghz_devices_start_async_rx(
        reader->device, subghz_worker_rx_callback, reader->worker);
    subghz_worker_start(reader->worker);

    reader->running = true;

    FURI_LOG_I(
        TAG, "Worker/receiver RX started at %lu Hz",
        (unsigned long)reader->frequency);

    return true;
}

void subghz_reader_stop(SubGhzReader* reader) {
    if(!reader || !reader->device || !reader->running) return;

    if(subghz_worker_is_running(reader->worker)) {
        subghz_worker_stop(reader->worker);
        subghz_devices_stop_async_rx(reader->device);
    }

    subghz_devices_idle(reader->device);
    reader->running = false;
}

void subghz_reader_get_status(
    SubGhzReader* reader,
    SubGhzReaderStatus* status) {
    if(!reader || !status) return;

    FURI_CRITICAL_ENTER();
    status->running = reader->running;
    status->frequency = reader->frequency;
    status->valid_frames = reader->valid_frames;
    status->sequence = reader->sequence;
    status->protocol = reader->protocol;
    status->native_keeloq_frames = reader->native_keeloq_frames;
    status->factory_code = reader->factory_code;
    status->function_code = reader->function_code;
    status->fixed32 = reader->fixed32;
    FURI_CRITICAL_EXIT();

    status->rssi =
        status->running ?
            subghz_devices_get_rssi(reader->device) :
            -127.0f;
}
