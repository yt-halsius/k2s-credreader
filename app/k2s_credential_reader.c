#include <furi.h>
#include <assets_icons.h>
#include <gui/gui.h>
#include <gui/elements.h>
#include <gui/view_port.h>
#include <input/input.h>

#include "aptus.h"
#include "contactless.h"
#include "credential.h"
#include "credential_feedback.h"
#include "lf_reader.h"
#include "hid_output.h"
#include "nfc_reader.h"
#include "rco_leb.h"
#include "subghz_reader.h"

#include <stdlib.h>
#include <string.h>

#define UI_REFRESH_MS 50U

/* Contactless scan timing. */
#define EM_SCAN_WINDOW_MS 1200U
#define MIFARE_SCAN_WINDOW_MS 500U
#define RELEASE_MISS_WINDOWS 2U

typedef enum {
    K2sModeContactless = 0,
    K2sModeRadio,
} K2sMode;

typedef enum {
    ContactlessStateScanEm = 0,
    ContactlessStateScanMifare,
} ContactlessState;

typedef struct {
    FuriMessageQueue* input_queue;
    ViewPort* view_port;
    Gui* gui;

    CredentialFeedback* feedback;
    HidOutput* hid;
    bool hid_enable_ok;
    bool mifare_lsb_first;

    K2sMode mode;
    ContactlessState contactless_state;
    uint32_t state_started_tick;
    bool state_saw_credential;

    LfReader* lf;
    NfcReader* nfc;
    SubGhzReader* subghz;

    bool lf_ok;
    bool nfc_ok;
    bool radio_ok;

    LfReaderStatus lf_status;
    NfcReaderStatus nfc_status;
    SubGhzReaderStatus radio_status;

    uint32_t last_lf_sequence;
    uint32_t last_nfc_sequence;
    uint32_t last_radio_sequence;

    Credential em;
    Credential mifare;
    Credential radio_credential;

    bool have_em;
    bool have_mifare;
    bool have_radio;

    uint8_t em_miss_windows;
    uint8_t mifare_miss_windows;

    bool debug;
} K2sApp;

static bool credential_same(const Credential* a, const Credential* b) {
    if(!a || !b) return false;
    return a->type == b->type &&
           a->button == b->button &&
           strcmp(a->value, b->value) == 0;
}

static void reverse_current_mifare_value(K2sApp* app) {
    if(!app || !app->have_mifare) return;

    const char* src = app->mifare.value;
    if(strlen(src) != 10U || src[0] != '0' || src[1] != '0') return;

    char reversed[11] = {
        '0', '0',
        src[8], src[9],
        src[6], src[7],
        src[4], src[5],
        src[2], src[3],
        '\0',
    };

    memcpy(app->mifare.value, reversed, sizeof(reversed));
}

static void output_credential(K2sApp* app, const Credential* credential) {
    if(!app || !credential) return;

    credential_feedback_emit(app->feedback, credential);

    if(app->hid && hid_output_is_enabled(app->hid)) {
        hid_output_send_credential(app->hid, credential);
    }
}

static void stop_lf(K2sApp* app) {
    if(app->lf) {
        lf_reader_free(app->lf);
        app->lf = NULL;
    }
    app->lf_ok = false;
    app->last_lf_sequence = 0;
    memset(&app->lf_status, 0, sizeof(app->lf_status));
}

static void stop_nfc(K2sApp* app) {
    if(app->nfc) {
        nfc_reader_free(app->nfc);
        app->nfc = NULL;
    }
    app->nfc_ok = false;
    app->last_nfc_sequence = 0;
    memset(&app->nfc_status, 0, sizeof(app->nfc_status));
}

static void stop_contactless(K2sApp* app) {
    stop_lf(app);
    stop_nfc(app);
}

static void stop_radio(K2sApp* app) {
    if(app->subghz) {
        subghz_reader_free(app->subghz);
        app->subghz = NULL;
    }
    app->radio_ok = false;
    app->last_radio_sequence = 0;
    memset(&app->radio_status, 0, sizeof(app->radio_status));
    app->radio_status.rssi = -127.0f;
}

static bool start_em_scan(K2sApp* app) {
    stop_nfc(app);
    stop_lf(app);

    app->lf = lf_reader_alloc();
    if(!app->lf) {
        app->lf_ok = false;
        return false;
    }

    app->lf_ok = lf_reader_start(app->lf);
    if(!app->lf_ok) {
        lf_reader_free(app->lf);
        app->lf = NULL;
        return false;
    }

    app->contactless_state = ContactlessStateScanEm;
    app->state_started_tick = furi_get_tick();
    app->state_saw_credential = false;
    app->last_lf_sequence = 0;
    return true;
}

static bool start_mifare_scan(K2sApp* app) {
    stop_lf(app);
    stop_nfc(app);

    app->nfc = nfc_reader_alloc();
    if(!app->nfc) {
        app->nfc_ok = false;
        return false;
    }

    app->nfc_ok = nfc_reader_start(app->nfc);
    if(!app->nfc_ok) {
        nfc_reader_free(app->nfc);
        app->nfc = NULL;
        return false;
    }

    app->contactless_state = ContactlessStateScanMifare;
    app->state_started_tick = furi_get_tick();
    app->state_saw_credential = false;
    app->last_nfc_sequence = 0;
    return true;
}

static bool start_contactless(K2sApp* app) {
    stop_radio(app);
    stop_contactless(app);

    app->mode = K2sModeContactless;
    app->have_radio = false;
    credential_clear(&app->radio_credential);

    /* EM always gets the first scan window. */
    return start_em_scan(app);
}

static bool start_radio(K2sApp* app) {
    stop_contactless(app);
    stop_radio(app);

    app->mode = K2sModeRadio;
    app->subghz = subghz_reader_alloc();
    if(!app->subghz) {
        app->radio_ok = false;
        return false;
    }

    app->radio_ok = subghz_reader_start(app->subghz);
    if(!app->radio_ok) {
        subghz_reader_free(app->subghz);
        app->subghz = NULL;
        return false;
    }

    return true;
}

static void accept_em(K2sApp* app, const Credential* credential) {
    app->state_saw_credential = true;
    app->em_miss_windows = 0;

    /*
     * EM and MIFARE have independent duplicate state.
     * A new EM must never invalidate or re-arm the current MIFARE value.
     */
    if(!app->have_em || !credential_same(&app->em, credential)) {
        app->em = *credential;
        app->have_em = true;
        output_credential(app, &app->em);
    }
}

static void accept_mifare(K2sApp* app, const Credential* credential) {
    app->state_saw_credential = true;
    app->mifare_miss_windows = 0;

    if(!app->have_mifare || !credential_same(&app->mifare, credential)) {
        app->mifare = *credential;
        app->have_mifare = true;
        output_credential(app, &app->mifare);
    }
}

static void update_em_scan(K2sApp* app) {
    if(!app->lf_ok || !app->lf) return;

    lf_reader_get_status(app->lf, &app->lf_status);

    if(app->lf_status.sequence != 0 &&
       app->lf_status.sequence != app->last_lf_sequence) {
        app->last_lf_sequence = app->lf_status.sequence;

        Credential credential = {0};
        if(app->lf_status.data_len == K2S_EM4100_DATA_LEN &&
           contactless_decode_em4100(app->lf_status.data, &credential)) {
            accept_em(app, &credential);

            /*
             * EM has priority. Once a valid EM credential is obtained there is
             * no reason to keep the LF field up for the remainder of the
             * window: tear it down and immediately give NFC its chance.
             *
             * This is also important for hardware bring-up. The native LF
             * worker needs appreciably more settling/decoding time than the
             * NFC poller, so short symmetric windows can starve EM while NFC
             * continues to work.
             */
            start_mifare_scan(app);
            return;
        }
    }

    if(furi_get_tick() - app->state_started_tick >=
       furi_ms_to_ticks(EM_SCAN_WINDOW_MS)) {

        if(!app->state_saw_credential && app->have_em) {
            if(app->em_miss_windows < UINT8_MAX) app->em_miss_windows++;
            if(app->em_miss_windows >= RELEASE_MISS_WINDOWS) {
                /*
                 * Re-arm EM only. MIFARE state is independent and remains
                 * unchanged until MIFARE itself is released or changes.
                 */
                app->have_em = false;
                credential_clear(&app->em);
                app->em_miss_windows = 0;
            }
        }

        start_mifare_scan(app);
    }
}

static void update_mifare_scan(K2sApp* app) {
    if(!app->nfc_ok || !app->nfc) return;

    nfc_reader_get_status(app->nfc, &app->nfc_status);

    if(app->nfc_status.sequence != 0 &&
       app->nfc_status.sequence != app->last_nfc_sequence) {
        app->last_nfc_sequence = app->nfc_status.sequence;

        Credential credential = {0};
        if(contactless_decode_mifare_uid_order(
               app->nfc_status.uid,
               app->nfc_status.uid_len,
               app->mifare_lsb_first,
               &credential)) {
            accept_mifare(app, &credential);
        }
    }

    if(furi_get_tick() - app->state_started_tick >=
       furi_ms_to_ticks(MIFARE_SCAN_WINDOW_MS)) {

        if(!app->state_saw_credential && app->have_mifare) {
            if(app->mifare_miss_windows < UINT8_MAX) app->mifare_miss_windows++;
            if(app->mifare_miss_windows >= RELEASE_MISS_WINDOWS) {
                /*
                 * Re-arm MIFARE only. EM state is independent and remains
                 * unchanged until EM itself is released or changes.
                 */
                app->have_mifare = false;
                credential_clear(&app->mifare);
                app->mifare_miss_windows = 0;
            }
        }

        start_em_scan(app);
    }
}

static void update_radio(K2sApp* app) {
    if(!app->radio_ok || !app->subghz) return;

    subghz_reader_get_status(app->subghz, &app->radio_status);

    if(app->radio_status.sequence == 0 ||
       app->radio_status.sequence == app->last_radio_sequence) {
        return;
    }

    app->last_radio_sequence = app->radio_status.sequence;

    Credential credential = {0};

    switch(app->radio_status.protocol) {
    case SubGhzReaderProtocolRcoLeb: {
        const uint8_t channel = app->radio_status.function_code + 1U;
        RcoLebCredential rco = {0};

        if(rco_leb_decode_factory(app->radio_status.factory_code, channel, &rco) &&
           credential_set(
               &credential,
               CredentialTypeRcoRadio,
               rco.rco_id,
               channel)) {

            if(!app->have_radio || !credential_same(&app->radio_credential, &credential)) {
                app->radio_credential = credential;
                app->have_radio = true;
                output_credential(app, &app->radio_credential);
            }
        }
        break;
    }

    case SubGhzReaderProtocolAptusKeeLoq:
        if(aptus_decode_fixed32(app->radio_status.fixed32, &credential)) {
            if(!app->have_radio || !credential_same(&app->radio_credential, &credential)) {
                app->radio_credential = credential;
                app->have_radio = true;
                output_credential(app, &app->radio_credential);
            }
        }
        break;

    case SubGhzReaderProtocolNone:
    default:
        break;
    }
}

static void draw_center_action_button(Canvas* canvas, const char* label, int32_t x_shift) {
    /*
     * Native elements_button_center() is exactly centered. With three footer
     * controls the center MSB/LSB button lands immediately next to HID OFF.
     * Draw the same native geometry a few pixels left to create visual air.
     */
    const size_t button_height = 12U;
    const size_t vertical_offset = 3U;
    const size_t horizontal_offset = 1U;
    const size_t icon_width = 7U;
    const size_t icon_height = 7U;
    const size_t icon_h_offset = 3U;
    const size_t icon_width_with_offset = icon_width + icon_h_offset;
    const size_t string_width = canvas_string_width(canvas, label);
    const size_t button_width =
        string_width + horizontal_offset * 2U + icon_width_with_offset;

    const int32_t x =
        ((int32_t)canvas_width(canvas) - (int32_t)button_width) / 2 + x_shift;
    const int32_t y = (int32_t)canvas_height(canvas);

    canvas_draw_box(canvas, x, y - (int32_t)button_height, button_width, button_height);

    canvas_draw_line(
        canvas, x - 1, y, x - 1, y - (int32_t)button_height);
    canvas_draw_line(
        canvas, x - 2, y, x - 2, y - (int32_t)button_height + 1);
    canvas_draw_line(
        canvas, x - 3, y, x - 3, y - (int32_t)button_height + 2);

    canvas_draw_line(
        canvas,
        x + (int32_t)button_width,
        y,
        x + (int32_t)button_width,
        y - (int32_t)button_height);
    canvas_draw_line(
        canvas,
        x + (int32_t)button_width + 1,
        y,
        x + (int32_t)button_width + 1,
        y - (int32_t)button_height + 1);
    canvas_draw_line(
        canvas,
        x + (int32_t)button_width + 2,
        y,
        x + (int32_t)button_width + 2,
        y - (int32_t)button_height + 2);

    canvas_invert_color(canvas);
    canvas_draw_icon(
        canvas,
        x + (int32_t)horizontal_offset,
        y - (int32_t)(icon_height + vertical_offset),
        &I_ButtonCenter_7x7);
    canvas_draw_str(
        canvas,
        x + (int32_t)horizontal_offset + (int32_t)icon_width_with_offset,
        y - (int32_t)vertical_offset,
        label);
    canvas_invert_color(canvas);
}

static void draw_footer(Canvas* canvas, K2sApp* app) {
    /*
     * Reserve the bottom native button-label area exclusively for controls.
     * Button text describes the action.
     */
    canvas_set_font(canvas, FontSecondary);

    elements_button_left(
        canvas,
        app->mode == K2sModeRadio ? "Contact" : "Radio");

    /*
     * MIFARE byte order only applies in Contactless mode.
     * LSB is the default, so the action offered at startup is MSB.
     */
    if(app->mode == K2sModeContactless) {
        draw_center_action_button(
            canvas,
            app->mifare_lsb_first ? "MSB" : "LSB",
            -4);
    }

    elements_button_right(
        canvas,
        (app->hid && hid_output_is_enabled(app->hid)) ? "HID OFF" : "HID ON");
}

static void draw_contactless(Canvas* canvas, K2sApp* app) {
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 22, "CONTACTLESS");

    if(!app->lf_ok && app->contactless_state == ContactlessStateScanEm) {
        canvas_draw_str(canvas, 2, 37, "EM reader start FAILED");
        return;
    }

    if(!app->nfc_ok && app->contactless_state == ContactlessStateScanMifare) {
        canvas_draw_str(canvas, 2, 37, "NFC reader start FAILED");
        return;
    }

    if(app->have_em && app->have_mifare) {
        canvas_draw_str(canvas, 2, 29, "EM");
        canvas_draw_str(canvas, 20, 29, app->em.value);
        canvas_draw_str(canvas, 2, 43, "MF");
        canvas_draw_str(canvas, 20, 43, app->mifare.value);
    } else if(app->have_em) {
        canvas_draw_str(canvas, 2, 29, "EM");
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 2, 45, app->em.value);
    } else if(app->have_mifare) {
        canvas_draw_str(canvas, 2, 29, "MIFARE");
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 2, 45, app->mifare.value);
    } else {
        canvas_draw_str(
            canvas,
            2,
            36,
            app->contactless_state == ContactlessStateScanEm ?
                "Scanning EM..." :
                "Scanning MIFARE...");
    }

}

static void draw_radio(Canvas* canvas, K2sApp* app) {
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 22, "RADIO");

    if(!app->radio_ok) {
        canvas_draw_str(canvas, 2, 34, "CC1101 RX start FAILED");
        return;
    }

    if(!app->have_radio) {
        canvas_draw_str(canvas, 2, 34, "Listening RCO / Aptus...");
        return;
    }

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 31, credential_type_name(app->radio_credential.type));
    canvas_set_font(canvas, FontBigNumbers);
    canvas_draw_str(canvas, 2, 49, app->radio_credential.value);
}

static void draw_debug(Canvas* canvas, K2sApp* app) {
    char line[40];

    canvas_set_font(canvas, FontSecondary);
    snprintf(
        line,
        sizeof(line),
        "Mode:%s",
        app->mode == K2sModeRadio ? "RADIO" : "CONTACTLESS");
    canvas_draw_str(canvas, 2, 22, line);

    if(app->mode == K2sModeRadio) {
        snprintf(
            line,
            sizeof(line),
            "RSSI %.0f  V:%lu",
            (double)app->radio_status.rssi,
            (unsigned long)app->radio_status.valid_frames);
        canvas_draw_str(canvas, 2, 38, line);

        snprintf(
            line,
            sizeof(line),
            "KeeLoq:%lu",
            (unsigned long)app->radio_status.native_keeloq_frames);
        canvas_draw_str(canvas, 2, 49, line);
    } else {
        snprintf(
            line,
            sizeof(line),
            "State:%s",
            app->contactless_state == ContactlessStateScanEm ? "EM" : "MIFARE");
        canvas_draw_str(canvas, 2, 38, line);

        snprintf(
            line,
            sizeof(line),
            "EM:%s MF:%s",
            app->have_em ? "yes" : "no",
            app->have_mifare ? "yes" : "no");
        canvas_draw_str(canvas, 2, 49, line);
    }

    canvas_draw_str(canvas, 2, 54, app->mifare_lsb_first ? "UID:LSB  UP normal" : "UID:MSB  UP normal");
}

static void draw_cb(Canvas* canvas, void* context) {
    K2sApp* app = context;

    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 11, "K2S Credential Reader");

    if(app->debug) {
        draw_debug(canvas, app);
    } else if(app->mode == K2sModeRadio) {
        draw_radio(canvas, app);
    } else {
        draw_contactless(canvas, app);
    }

    draw_footer(canvas, app);
}

static void input_cb(InputEvent* event, void* context) {
    K2sApp* app = context;
    furi_message_queue_put(app->input_queue, event, 0);
}

int32_t k2s_credential_reader_app(void* p) {
    UNUSED(p);

    K2sApp* app = malloc(sizeof(K2sApp));
    if(!app) return -1;

    memset(app, 0, sizeof(K2sApp));
    credential_clear(&app->em);
    credential_clear(&app->mifare);
    credential_clear(&app->radio_credential);
    app->radio_status.rssi = -127.0f;
    app->mifare_lsb_first = true;

    app->feedback = credential_feedback_alloc();

    /*
     * HID keyboard output is ON by default. The previous USB configuration is
     * saved by HidOutput and restored when HID is disabled or the app exits.
     */
    app->hid = hid_output_alloc();
    if(app->hid) {
        app->hid_enable_ok = hid_output_set_enabled(app->hid, true);
    }

    app->input_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, draw_cb, app);
    view_port_input_callback_set(app->view_port, input_cb, app);

    app->gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    start_contactless(app);

    bool running = true;
    InputEvent event;

    while(running) {
        if(furi_message_queue_get(
               app->input_queue,
               &event,
               furi_ms_to_ticks(UI_REFRESH_MS)) == FuriStatusOk) {
            if(event.type == InputTypeShort) {
                if(event.key == InputKeyBack) {
                    running = false;
                } else if(event.key == InputKeyOk) {
                    app->mifare_lsb_first = !app->mifare_lsb_first;
                    reverse_current_mifare_value(app);
                } else if(event.key == InputKeyUp) {
                    app->debug = !app->debug;
                } else if(event.key == InputKeyLeft) {
                    if(app->mode == K2sModeRadio) {
                        start_contactless(app);
                    } else {
                        start_radio(app);
                    }
                } else if(event.key == InputKeyRight) {
                    if(app->hid) {
                        const bool next_enabled = !hid_output_is_enabled(app->hid);
                        app->hid_enable_ok =
                            hid_output_set_enabled(app->hid, next_enabled);
                    }
                }
            }
        }

        if(app->mode == K2sModeRadio) {
            update_radio(app);
        } else if(app->contactless_state == ContactlessStateScanEm) {
            update_em_scan(app);
        } else {
            update_mifare_scan(app);
        }

        view_port_update(app->view_port);
    }

    stop_contactless(app);
    stop_radio(app);

    gui_remove_view_port(app->gui, app->view_port);
    furi_record_close(RECORD_GUI);

    view_port_free(app->view_port);
    furi_message_queue_free(app->input_queue);

    if(app->feedback) {
        credential_feedback_free(app->feedback);
    }

    if(app->hid) {
        hid_output_free(app->hid);
        app->hid = NULL;
    }

    memset(app, 0, sizeof(K2sApp));
    free(app);
    return 0;
}
