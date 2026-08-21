CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2

TARGET := test_rco_leb
SOURCES := test_rco_leb.c ../app/rco_leb.c

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -I../app $(SOURCES) -o $(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
