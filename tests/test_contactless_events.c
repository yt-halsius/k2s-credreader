#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../app/credential.h"

typedef struct {
    Credential em;
    Credential mifare;
    bool have_em;
    bool have_mifare;
    unsigned e_events;
    unsigned m_events;
} State;

static bool same(const Credential* a, const Credential* b) {
    return a->type == b->type &&
           a->button == b->button &&
           strcmp(a->value, b->value) == 0;
}

static void accept_em(State* s, const Credential* c) {
    if(!s->have_em || !same(&s->em, c)) {
        s->em = *c;
        s->have_em = true;
        s->e_events++;
    }
}

static void accept_mifare(State* s, const Credential* c) {
    if(!s->have_mifare || !same(&s->mifare, c)) {
        s->mifare = *c;
        s->have_mifare = true;
        s->m_events++;
    }
}

static void must(bool ok, const char* msg) {
    if(!ok) {
        fprintf(stderr, "FAIL: %s\n", msg);
        exit(EXIT_FAILURE);
    }
}

int main(void) {
    State s = {0};

    Credential em_a = {
        .type = CredentialTypeEm,
        .value = "00123456789A",
        .button = 0,
    };
    Credential em_b = {
        .type = CredentialTypeEm,
        .value = "00123456789B",
        .button = 0,
    };
    Credential mf_a = {
        .type = CredentialTypeMifare,
        .value = "0012345678",
        .button = 0,
    };
    Credential mf_b = {
        .type = CredentialTypeMifare,
        .value = "0012345679",
        .button = 0,
    };

    /* EM first, then MIFARE; repeats are silent. */
    accept_em(&s, &em_a);
    accept_mifare(&s, &mf_a);
    accept_em(&s, &em_a);
    accept_mifare(&s, &mf_a);

    must(s.e_events == 1, "same EM must not re-emit E");
    must(s.m_events == 1, "same MIFARE must not re-emit M");

    /* A new EM must not clear/re-arm MIFARE. */
    accept_em(&s, &em_b);
    accept_mifare(&s, &mf_a);

    must(s.e_events == 2, "new EM must emit E");
    must(s.m_events == 1, "new EM must not re-emit unchanged MIFARE");

    /* A new MIFARE must not clear/re-arm EM. */
    accept_mifare(&s, &mf_b);
    accept_em(&s, &em_b);

    must(s.m_events == 2, "new MIFARE must emit M");
    must(s.e_events == 2, "new MIFARE must not re-emit unchanged EM");

    puts("All contactless event-state tests passed.");
    return EXIT_SUCCESS;
}
