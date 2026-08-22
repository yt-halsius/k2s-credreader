#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contactless.h"

static void require(bool condition, const char* message) {
    if(!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

int main(void) {
    Credential credential = {0};

    const uint8_t em[K2S_EM4100_DATA_LEN] = {0x12, 0x34, 0x56, 0x78, 0x9A};
    require(contactless_decode_em4100(em, &credential), "EM decode");
    require(credential.type == CredentialTypeEm, "EM type");
    require(strcmp(credential.value, "00123456789A") == 0, "EM formatting");
    require(credential.button == 0U, "EM button must be zero");

    const uint8_t mifare[K2S_MIFARE_UID_LEN] = {0x12, 0x34, 0x56, 0x78};
    require(contactless_decode_mifare_uid(mifare, sizeof(mifare), &credential), "MIFARE decode");
    require(credential.type == CredentialTypeMifare, "MIFARE type");
    require(strcmp(credential.value, "0012345678") == 0, "MIFARE formatting");
    require(credential.button == 0U, "MIFARE button must be zero");

    const uint8_t long_uid[7] = {1, 2, 3, 4, 5, 6, 7};
    require(!contactless_decode_mifare_uid(long_uid, sizeof(long_uid), &credential), "reject 7-byte UID");
    require(!contactless_decode_mifare_uid(NULL, 4U, &credential), "reject null UID");
    require(!contactless_decode_mifare_uid(mifare, 3U, &credential), "reject short UID");
    require(!contactless_decode_em4100(NULL, &credential), "reject null EM");

    puts("All EM/MIFARE normalization tests passed.");
    return EXIT_SUCCESS;
}
