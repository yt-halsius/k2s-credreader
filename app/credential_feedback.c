#include "credential_feedback.h"

#include <furi.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>
#include <stdlib.h>
#include <string.h>

#define FEEDBACK_DEBOUNCE_MS 2500U
#define MORSE_FREQUENCY_HZ 1000.0f
#define MORSE_VOLUME 1.0f
#define MORSE_DOT_MS 40U
#define MORSE_DASH_MS 120U
#define MORSE_GAP_MS 40U

/*
 * Custom NotificationMessage objects let us use exact Morse timing instead
 * of being limited to the predefined 25/50/100 ms notification delays.
 * The Notification service still owns the speaker and applies the normal
 * Flipper sound settings.
 */
static const NotificationMessage message_morse_tone = {
    .type = NotificationMessageTypeSoundOn,
    .data.sound = {
        .frequency = MORSE_FREQUENCY_HZ,
        .volume = MORSE_VOLUME,
    },
};

static const NotificationMessage message_morse_dot = {
    .type = NotificationMessageTypeDelay,
    .data.delay = {
        .length = MORSE_DOT_MS,
    },
};

static const NotificationMessage message_morse_dash = {
    .type = NotificationMessageTypeDelay,
    .data.delay = {
        .length = MORSE_DASH_MS,
    },
};

static const NotificationMessage message_morse_gap = {
    .type = NotificationMessageTypeDelay,
    .data.delay = {
        .length = MORSE_GAP_MS,
    },
};

static const NotificationSequence sequence_morse_r = {
    &message_morse_tone, &message_morse_dot, &message_sound_off, &message_morse_gap,
    &message_morse_tone, &message_morse_dash, &message_sound_off, &message_morse_gap,
    &message_morse_tone, &message_morse_dot, &message_sound_off, NULL,
};

static const NotificationSequence sequence_morse_a = {
    &message_morse_tone, &message_morse_dot, &message_sound_off, &message_morse_gap,
    &message_morse_tone, &message_morse_dash, &message_sound_off, NULL,
};

/* Reserved for the upcoming MIFARE and EM adapters. */
static const NotificationSequence sequence_morse_m = {
    &message_morse_tone, &message_morse_dash, &message_sound_off, &message_morse_gap,
    &message_morse_tone, &message_morse_dash, &message_sound_off, NULL,
};

static const NotificationSequence sequence_morse_e = {
    &message_morse_tone, &message_morse_dot, &message_sound_off, NULL,
};

struct CredentialFeedback {
    NotificationApp* notification;
    CredentialType last_type;
    char last_value[CREDENTIAL_VALUE_MAX];
    uint8_t last_button;
    uint32_t last_tick;
    bool have_last;
};

const char* credential_feedback_morse(CredentialType type) {
    switch(type) {
    case CredentialTypeRcoRadio: return ".-.";
    case CredentialTypeAptusRadio: return ".-";
    case CredentialTypeMifare: return "--";
    case CredentialTypeEm: return ".";
    default: return "";
    }
}

static const NotificationSequence* feedback_sequence(CredentialType type) {
    switch(type) {
    case CredentialTypeRcoRadio: return &sequence_morse_r;
    case CredentialTypeAptusRadio: return &sequence_morse_a;
    case CredentialTypeMifare: return &sequence_morse_m;
    case CredentialTypeEm: return &sequence_morse_e;
    default: return NULL;
    }
}

CredentialFeedback* credential_feedback_alloc(void) {
    CredentialFeedback* feedback = malloc(sizeof(CredentialFeedback));
    if(!feedback) return NULL;
    memset(feedback, 0, sizeof(CredentialFeedback));
    feedback->notification = furi_record_open(RECORD_NOTIFICATION);
    return feedback;
}

void credential_feedback_free(CredentialFeedback* feedback) {
    if(!feedback) return;
    if(feedback->notification) {
        notification_message_block(feedback->notification, &sequence_reset_sound);
        furi_record_close(RECORD_NOTIFICATION);
    }
    memset(feedback, 0, sizeof(CredentialFeedback));
    free(feedback);
}

void credential_feedback_emit(CredentialFeedback* feedback, const Credential* credential) {
    if(!feedback || !feedback->notification || !credential || credential->value[0] == '\0') return;

    const NotificationSequence* sequence = feedback_sequence(credential->type);
    if(!sequence) return;

    const uint32_t now = furi_get_tick();
    const bool same =
        feedback->have_last &&
        feedback->last_type == credential->type &&
        feedback->last_button == credential->button &&
        strcmp(feedback->last_value, credential->value) == 0;

    if(same && (now - feedback->last_tick) < furi_ms_to_ticks(FEEDBACK_DEBOUNCE_MS)) return;

    feedback->last_type = credential->type;
    feedback->last_button = credential->button;
    strncpy(feedback->last_value, credential->value, sizeof(feedback->last_value) - 1U);
    feedback->last_value[sizeof(feedback->last_value) - 1U] = '\0';
    feedback->last_tick = now;
    feedback->have_last = true;

    notification_message(feedback->notification, sequence);
}
