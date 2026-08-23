CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2

RCO_TARGET := test_rco_leb
RCO_SOURCES := test_rco_leb.c ../app/rco_leb.c

APTUS_TARGET := test_aptus
APTUS_SOURCES := test_aptus.c ../app/aptus.c ../app/credential.c

CONTACTLESS_TARGET := test_contactless
EVENT_TARGET := test_contactless_events
CONTACTLESS_SOURCES := test_contactless.c ../app/contactless.c ../app/credential.c
EVENT_SOURCES := test_contactless_events.c ../app/credential.c

.PHONY: all test clean

all: $(RCO_TARGET) $(APTUS_TARGET) $(CONTACTLESS_TARGET) $(EVENT_TARGET)

$(RCO_TARGET): $(RCO_SOURCES)
	$(CC) $(CFLAGS) -I../app $(RCO_SOURCES) -o $(RCO_TARGET)

$(APTUS_TARGET): $(APTUS_SOURCES)
	$(CC) $(CFLAGS) -I../app $(APTUS_SOURCES) -o $(APTUS_TARGET)

$(CONTACTLESS_TARGET): $(CONTACTLESS_SOURCES)
	$(CC) $(CFLAGS) -I../app $(CONTACTLESS_SOURCES) -o $(CONTACTLESS_TARGET)

$(EVENT_TARGET): $(EVENT_SOURCES)
	$(CC) $(CFLAGS) -I../app $(EVENT_SOURCES) -o $(EVENT_TARGET)

test: all
	./$(RCO_TARGET)
	./$(APTUS_TARGET)
	./$(CONTACTLESS_TARGET)
	./$(EVENT_TARGET)

clean:
	rm -f $(RCO_TARGET) $(APTUS_TARGET) $(CONTACTLESS_TARGET) $(EVENT_TARGET)
