#pragma once

#include <stdbool.h>
#include <stdint.h>

#define CREDENTIAL_VALUE_MAX 32U

typedef enum {
    CredentialTypeNone = 0,
    CredentialTypeRcoRadio,
    CredentialTypeAptusRadio,
    CredentialTypeEm,
    CredentialTypeMifare,
} CredentialType;

typedef struct {
    CredentialType type;
    char value[CREDENTIAL_VALUE_MAX];
    uint8_t button;
} Credential;

void credential_clear(Credential* credential);
bool credential_set(Credential* credential, CredentialType type, const char* value, uint8_t button);
const char* credential_type_name(CredentialType type);
