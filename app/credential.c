#include "credential.h"

#include <string.h>

void credential_clear(Credential* credential) {
    if(!credential) return;
    memset(credential, 0, sizeof(Credential));
}

bool credential_set(Credential* credential, CredentialType type, const char* value, uint8_t button) {
    if(!credential || !value) return false;

    const size_t len = strlen(value);
    if(len == 0U || len >= CREDENTIAL_VALUE_MAX) return false;

    credential_clear(credential);
    credential->type = type;
    credential->button = button;
    memcpy(credential->value, value, len + 1U);
    return true;
}

const char* credential_type_name(CredentialType type) {
    switch(type) {
    case CredentialTypeRcoRadio:
        return "RCO Radio";
    case CredentialTypeAptusRadio:
        return "Aptus Radio";
    case CredentialTypeEm:
        return "EM";
    case CredentialTypeMifare:
        return "MIFARE";
    case CredentialTypeNone:
    default:
        return "Unknown";
    }
}
