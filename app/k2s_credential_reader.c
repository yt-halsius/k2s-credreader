#include <furi.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <input/input.h>

#include "credential.h"
#include "rco_leb.h"
#include "subghz_reader.h"

#define UI_REFRESH_MS 100U

typedef struct {
    FuriMessageQueue* input_queue;
    ViewPort* view_port;
    Gui* gui;
    SubGhzReader* subghz;
    bool decoder_self_test_ok;
    bool rx_start_ok;
    SubGhzReaderStatus radio;
    uint32_t last_sequence;
    bool have_credential;
    Credential credential;
    uint32_t rco_factory_code;
    uint8_t rco_function_code;
    bool debug_view;
} K2sReaderApp;

static bool k2s_decoder_self_test(void) {
    RcoLebCredential rco;
    return rco_leb_decode_34bit("1111111000010010001101000101011000", 1U, &rco) &&
           strcmp(rco.rco_id, "005222016") == 0;
}

static void k2s_update(K2sReaderApp* app) {
    if(!app->subghz) return;
    subghz_reader_get_status(app->subghz, &app->radio);

    if(app->radio.sequence == 0U || app->radio.sequence == app->last_sequence) return;
    app->last_sequence = app->radio.sequence;

    const uint8_t channel = app->radio.function_code + 1U;
    RcoLebCredential rco = {0};

    if(rco_leb_decode_factory(app->radio.factory_code, channel, &rco)) {
        app->rco_factory_code = rco.factory_code;
        app->rco_function_code = app->radio.function_code;
        app->have_credential = credential_set(&app->credential, CredentialTypeRcoRadio, rco.rco_id, channel);
    }
}

static void k2s_draw_normal(Canvas* canvas, K2sReaderApp* app) {
    canvas_set_font(canvas, FontPrimary);

    if(!app->have_credential) {
        canvas_draw_str(canvas, 2, 25, "Listening...");
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 2, 41, "RCO / LEB 433.92 MHz");
        canvas_draw_str(canvas, 2, 58, "Read-only");
        return;
    }

    canvas_draw_str(canvas, 2, 25, credential_type_name(app->credential.type));
    canvas_set_font(canvas, FontBigNumbers);
    canvas_draw_str(canvas, 2, 49, app->credential.value);
    canvas_set_font(canvas, FontSecondary);

    char line[32];
    snprintf(line, sizeof(line), "Button %u", app->credential.button);
    canvas_draw_str(canvas, 2, 62, line);
}

static void k2s_draw_debug(Canvas* canvas, K2sReaderApp* app) {
    canvas_set_font(canvas, FontSecondary);
    char line[32];

    snprintf(line, sizeof(line), "RSSI %.0f dBm Valid %lu", (double)app->radio.rssi, (unsigned long)app->radio.valid_frames);
    canvas_draw_str(canvas, 2, 22, line);

    if(app->have_credential) {
        snprintf(line, sizeof(line), "Factory %06lX", (unsigned long)app->rco_factory_code);
        canvas_draw_str(canvas, 2, 34, line);
        snprintf(line, sizeof(line), "fn=%u -> Button %u", app->rco_function_code, app->credential.button);
        canvas_draw_str(canvas, 2, 46, line);
        canvas_draw_str(canvas, 2, 58, "ID");
        canvas_draw_str(canvas, 15, 58, app->credential.value);
    } else {
        canvas_draw_str(canvas, 2, 39, "Waiting for valid LEB...");
        canvas_draw_str(canvas, 2, 56, "No raw RF retained");
    }
}

static void k2s_draw_callback(Canvas* canvas, void* context) {
    K2sReaderApp* app = context;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "K2S Credential Reader");

    if(!app->decoder_self_test_ok) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 2, 32, "Decoder self-test FAILED");
        return;
    }
    if(!app->rx_start_ok) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 2, 32, "CC1101 RX start FAILED");
        return;
    }

    if(app->debug_view) k2s_draw_debug(canvas, app);
    else k2s_draw_normal(canvas, app);
}

static void k2s_input_callback(InputEvent* input_event, void* context) {
    K2sReaderApp* app = context;
    furi_message_queue_put(app->input_queue, input_event, 0);
}

int32_t k2s_credential_reader_app(void* p) {
    UNUSED(p);
    K2sReaderApp* app = malloc(sizeof(K2sReaderApp));
    if(!app) return -1;

    memset(app, 0, sizeof(K2sReaderApp));
    credential_clear(&app->credential);
    app->radio.rssi = -127.0f;
    app->decoder_self_test_ok = k2s_decoder_self_test();

    app->input_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, k2s_draw_callback, app);
    view_port_input_callback_set(app->view_port, k2s_input_callback, app);
    app->gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    if(app->decoder_self_test_ok) {
        app->subghz = subghz_reader_alloc();
        if(app->subghz) app->rx_start_ok = subghz_reader_start(app->subghz);
    }

    bool running = true;
    InputEvent event;
    while(running) {
        if(furi_message_queue_get(app->input_queue, &event, furi_ms_to_ticks(UI_REFRESH_MS)) == FuriStatusOk) {
            if(event.type == InputTypeShort) {
                if(event.key == InputKeyBack) running = false;
                else if(event.key == InputKeyOk) app->debug_view = !app->debug_view;
            }
        }
        k2s_update(app);
        view_port_update(app->view_port);
    }

    if(app->subghz) subghz_reader_free(app->subghz);
    gui_remove_view_port(app->gui, app->view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(app->view_port);
    furi_message_queue_free(app->input_queue);
    memset(app, 0, sizeof(K2sReaderApp));
    free(app);
    return 0;
}
