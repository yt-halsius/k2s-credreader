#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../app/aptus.h"

typedef struct {
    const char* name;
    uint32_t fixed32;
    uint8_t expected_button;
    uint32_t expected_serial28;
    const char* expected_id;
} AptusVector;

static int failures = 0;

static void expect_vector(const AptusVector* vector) {
    const uint32_t serial28 = aptus_serial28_from_fixed32(vector->fixed32);
    const uint8_t button = aptus_button_from_fixed32(vector->fixed32);

    if(serial28 != vector->expected_serial28) {
        fprintf(
            stderr,
            "FAIL %s serial28: expected %lu, got %lu\n",
            vector->name,
            (unsigned long)vector->expected_serial28,
            (unsigned long)serial28);
        failures++;
        return;
    }

    if(button != vector->expected_button) {
        fprintf(
            stderr,
            "FAIL %s button: expected %u, got %u\n",
            vector->name,
            vector->expected_button,
            button);
        failures++;
        return;
    }

    Credential credential = {0};

    if(!aptus_decode_fixed32(vector->fixed32, &credential)) {
        fprintf(stderr, "FAIL %s: decoder returned false\n", vector->name);
        failures++;
        return;
    }

    if(credential.type != CredentialTypeAptusRadio ||
       credential.button != vector->expected_button ||
       strcmp(credential.value, vector->expected_id) != 0) {
        fprintf(
            stderr,
            "FAIL %s: expected BTN%u %s, got BTN%u %s\n",
            vector->name,
            vector->expected_button,
            vector->expected_id,
            credential.button,
            credential.value);
        failures++;
        return;
    }

    printf(
        "PASS %-10s fixed=%08lX BTN%u -> %s\n",
        vector->name,
        (unsigned long)vector->fixed32,
        credential.button,
        credential.value);
}

static void expect_invalid_inputs(void) {
    Credential credential = {0};

    if(aptus_decode_fixed32(0x00000000U, &credential)) {
        fprintf(stderr, "FAIL unknown button code accepted\n");
        failures++;
    }

    if(aptus_decode_fixed32(0xE6A2C484U, NULL)) {
        fprintf(stderr, "FAIL NULL credential accepted\n");
        failures++;
    }
}

int main(void) {
    static const AptusVector vectors[] = {
        {"Tag1 BTN1", 0xE6A2C484U, 1U, 19088743U, "38177487"},
        {"Tag1 BTN2", 0xE6A2C482U, 2U, 19088743U, "38177486"},
        {"Tag2 BTN1", 0x1E6A2C44U, 1U, 36984440U, "73968881"},
        {"Tag2 BTN2", 0x1E6A2C42U, 2U, 36984440U, "73968880"},
        {"Tag3 BTN1", 0x91E6A2C4U, 1U, 54880137U, "109760275"},
        {"Tag3 BTN2", 0x91E6A2C2U, 2U, 54880137U, "109760274"},
        {"Tag4 BTN1", 0x591E6A24U, 1U, 72775834U, "145551669"},
        {"Tag4 BTN2", 0x591E6A22U, 2U, 72775834U, "145551668"},
    };

    puts("Aptus fixed32 conversion vectors");

    for(size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        expect_vector(&vectors[i]);
    }

    puts("\nInvalid input checks");
    expect_invalid_inputs();

    if(failures != 0) {
        fprintf(stderr, "\n%d test(s) failed\n", failures);
        return EXIT_FAILURE;
    }

    puts("\nAll Aptus tests passed.");
    return EXIT_SUCCESS;
}
