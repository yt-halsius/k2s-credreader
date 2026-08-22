#pragma once
#include <stdint.h>
#include "credential.h"

typedef struct CredentialFeedback CredentialFeedback;

CredentialFeedback* credential_feedback_alloc(void);
void credential_feedback_free(CredentialFeedback* feedback);
void credential_feedback_emit(CredentialFeedback* feedback, const Credential* credential);
const char* credential_feedback_morse(CredentialType type);
