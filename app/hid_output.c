#include "hid_output.h"

#include <furi.h>
#include <furi_hal_usb.h>
#include <furi_hal_usb_hid.h>
#include <usb_hid.h>

#include <stdlib.h>
#include <string.h>

/*
 * Credential values are deliberately restricted to decimal digits and
 * uppercase hexadecimal A-F by the normalization layer.
 *
 * HID keyboard usage IDs are layout-independent for the number row and the
 * Latin A-F keys. Uppercase A-F are emitted with Left Shift.
 */
#define HID_KEY_DELAY_MS 8U
#define HID_USB_REENUM_DELAY_MS 250U

struct HidOutput {
    FuriHalUsbInterface* previous_usb_config;
    bool enabled;
};

static bool hid_output_press_release(uint16_t key) {
    if(!furi_hal_hid_kb_press(key)) return false;
    furi_delay_ms(HID_KEY_DELAY_MS);
    const bool released = furi_hal_hid_kb_release(key);
    furi_delay_ms(HID_KEY_DELAY_MS);
    return released;
}

static bool hid_output_char_to_key(char c, uint16_t* key) {
    if(!key) return false;

    if(c >= '1' && c <= '9') {
        *key = (uint16_t)(HID_KEYBOARD_1 + (c - '1'));
        return true;
    }

    if(c == '0') {
        *key = HID_KEYBOARD_0;
        return true;
    }

    if(c >= 'A' && c <= 'F') {
        *key = (uint16_t)(HID_KEYBOARD_A + (c - 'A')) | KEY_MOD_LEFT_SHIFT;
        return true;
    }

    return false;
}

HidOutput* hid_output_alloc(void) {
    HidOutput* output = malloc(sizeof(HidOutput));
    if(!output) return NULL;

    memset(output, 0, sizeof(HidOutput));
    output->previous_usb_config = furi_hal_usb_get_config();
    return output;
}

bool hid_output_set_enabled(HidOutput* output, bool enabled) {
    if(!output) return false;

    if(enabled == output->enabled) return true;

    furi_hal_hid_kb_release_all();

    if(enabled) {
        if(!furi_hal_usb_set_config(&usb_hid, NULL)) {
            output->enabled = false;
            return false;
        }

        /*
         * Give the host a short opportunity to re-enumerate before credentials
         * can be emitted. is_connected() remains the final authority.
         */
        furi_delay_ms(HID_USB_REENUM_DELAY_MS);
        output->enabled = true;
        return true;
    }

    output->enabled = false;

    if(output->previous_usb_config && output->previous_usb_config != &usb_hid) {
        return furi_hal_usb_set_config(output->previous_usb_config, NULL);
    }

    return true;
}

bool hid_output_is_enabled(const HidOutput* output) {
    return output && output->enabled;
}

bool hid_output_is_connected(const HidOutput* output) {
    return output && output->enabled && furi_hal_hid_is_connected();
}

bool hid_output_send_credential(HidOutput* output, const Credential* credential) {
    if(!output || !output->enabled || !credential ||
       credential->type == CredentialTypeNone ||
       credential->value[0] == '\0' ||
       !furi_hal_hid_is_connected()) {
        return false;
    }

    for(const char* p = credential->value; *p != '\0'; ++p) {
        uint16_t key = 0;
        if(!hid_output_char_to_key(*p, &key)) {
            furi_hal_hid_kb_release_all();
            return false;
        }

        if(!hid_output_press_release(key)) {
            furi_hal_hid_kb_release_all();
            return false;
        }
    }

    if(!hid_output_press_release(HID_KEYBOARD_RETURN)) {
        furi_hal_hid_kb_release_all();
        return false;
    }

    furi_hal_hid_kb_release_all();
    return true;
}

void hid_output_free(HidOutput* output) {
    if(!output) return;

    furi_hal_hid_kb_release_all();

    if(output->enabled &&
       output->previous_usb_config &&
       output->previous_usb_config != &usb_hid) {
        furi_hal_usb_set_config(output->previous_usb_config, NULL);
    }

    memset(output, 0, sizeof(HidOutput));
    free(output);
}
