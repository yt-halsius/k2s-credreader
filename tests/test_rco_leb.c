#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../app/rco_leb.h"

typedef struct {
    uint32_t factory_code;
    uint8_t channel;
    const char* expected_id;
} FactoryVector;

typedef struct {
    const char* bits;
    uint8_t channel;
    const char* expected_id;
} BitVector;

static int failures = 0;

static void expect_factory(const FactoryVector* vector) {
    RcoLebCredential result = {0};

    if(!rco_leb_decode_factory(vector->factory_code, vector->channel, &result)) {
        fprintf(
            stderr,
            "FAIL factory=%06lX CH%u: decoder returned false\n",
            (unsigned long)vector->factory_code,
            vector->channel);
        failures++;
        return;
    }

    if(strcmp(result.rco_id, vector->expected_id) != 0) {
        fprintf(
            stderr,
            "FAIL factory=%06lX CH%u: expected %s, got %s\n",
            (unsigned long)vector->factory_code,
            vector->channel,
            vector->expected_id,
            result.rco_id);
        failures++;
        return;
    }

    if(result.factory_code != vector->factory_code || result.channel != vector->channel) {
        fprintf(
            stderr,
            "FAIL factory=%06lX CH%u: metadata mismatch\n",
            (unsigned long)vector->factory_code,
            vector->channel);
        failures++;
        return;
    }

    printf(
        "PASS factory=%06lX CH%u -> %s\n",
        (unsigned long)vector->factory_code,
        vector->channel,
        result.rco_id);
}

static void expect_bits(const BitVector* vector) {
    RcoLebCredential result = {0};

    if(!rco_leb_decode_34bit(vector->bits, vector->channel, &result)) {
        fprintf(stderr, "FAIL 34-bit vector CH%u: decoder returned false\n", vector->channel);
        failures++;
        return;
    }

    if(strcmp(result.rco_id, vector->expected_id) != 0) {
        fprintf(
            stderr,
            "FAIL 34-bit vector CH%u: expected %s, got %s\n",
            vector->channel,
            vector->expected_id,
            result.rco_id);
        failures++;
        return;
    }

    printf("PASS 34-bit CH%u -> %s\n", vector->channel, result.rco_id);
}

static void expect_invalid_inputs(void) {
    RcoLebCredential result = {0};

    if(rco_leb_decode_factory(0x123456U, 0U, &result)) {
        fprintf(stderr, "FAIL invalid channel 0 accepted\n");
        failures++;
    }

    if(rco_leb_decode_factory(0x123456U, 5U, &result)) {
        fprintf(stderr, "FAIL invalid channel 5 accepted\n");
        failures++;
    }

    if(rco_leb_decode_factory(0x1000000U, 1U, &result)) {
        fprintf(stderr, "FAIL >24-bit factory accepted\n");
        failures++;
    }

    if(rco_leb_decode_34bit(
           "0000000000010010001101000101011000", 1U, &result)) {
        fprintf(stderr, "FAIL invalid FE prefix accepted\n");
        failures++;
    }

    if(rco_leb_decode_34bit(
           "111111100001001000110100010101100", 1U, &result)) {
        fprintf(stderr, "FAIL 33-bit frame accepted\n");
        failures++;
    }
}

int main(void) {
    static const FactoryVector factory_vectors[] = {
        {0x123456U, 1U, "005222016"},
        {0x123456U, 2U, "005222017"},
        {0x123456U, 3U, "005222018"},
        {0x123456U, 4U, "005222019"},

        {0x234567U, 1U, "006926368"},
        {0x234567U, 2U, "006926369"},
        {0x234567U, 3U, "006926370"},
        {0x234567U, 4U, "006926371"},

        {0x345678U, 1U, "008630720"},
        {0x456789U, 1U, "010335072"},
        {0x56789AU, 1U, "012039424"},
        {0x6789ABU, 1U, "013743776"},
        {0x789ABCU, 1U, "015448128"},
        {0x89ABCDU, 1U, "017152480"},
    };

    static const BitVector bit_vectors[] = {
        {"1111111000010010001101000101011000", 1U, "005222016"},
        {"1111111000100011010001010110011100", 1U, "006926368"},
        {"1111111000110100010101100111100000", 1U, "008630720"},
        {"1111111001000101011001111000100100", 1U, "010335072"},
        {"1111111001010110011110001001101000", 1U, "012039424"},
        {"1111111001100111100010011010101100", 1U, "013743776"},
        {"1111111001111000100110101011110000", 1U, "015448128"},
        {"1111111010001001101010111100110100", 1U, "017152480"},
    };

    puts("RCO/LEB factory conversion vectors");
    for(size_t i = 0; i < sizeof(factory_vectors) / sizeof(factory_vectors[0]); i++) {
        expect_factory(&factory_vectors[i]);
    }

    puts("\nRCO/LEB 34-bit vectors");
    for(size_t i = 0; i < sizeof(bit_vectors) / sizeof(bit_vectors[0]); i++) {
        expect_bits(&bit_vectors[i]);
    }

    puts("\nInvalid input checks");
    expect_invalid_inputs();

    if(failures != 0) {
        fprintf(stderr, "\n%d test(s) failed\n", failures);
        return EXIT_FAILURE;
    }

    puts("\nAll RCO/LEB tests passed.");
    return EXIT_SUCCESS;
}
