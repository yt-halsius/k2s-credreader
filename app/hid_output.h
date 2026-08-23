#pragma once

#include <stdbool.h>
#include "credential.h"

typedef struct HidOutput HidOutput;

/*
 * USB HID keyboard output.
 *
 * Default policy belongs to the application; this layer only manages the USB
 * HID interface and emits an already-normalized Credential.value followed by
 * Enter.
 */
HidOutput* hid_output_alloc(void);
void hid_output_free(HidOutput* output);

bool hid_output_set_enabled(HidOutput* output, bool enabled);
bool hid_output_is_enabled(const HidOutput* output);
bool hid_output_is_connected(const HidOutput* output);

/* Returns false if disabled, disconnected, or a character could not be sent. */
bool hid_output_send_credential(HidOutput* output, const Credential* credential);
