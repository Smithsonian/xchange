# Use the parent directories for libraries and headers.
LIB = ../lib
INC = ../include

# Load the common Makefile definitions...
include ../config.mk

CFLAGS += -g

export CFLAGS

all: tests run

tests: $(BIN)/test-struct $(BIN)/test-lookup $(BIN)/test-json

run: tests
	$(BIN)/test-struct
	$(BIN)/test-lookup
	$(BIN)/test-json

$(BIN)/test-%: $(OBJ)/test-%.o $(LIB)/libxchange.a | $(BIN) Makefile
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean-test
clean-test:
	rm -rf bin

clean: clean-test

# Finally, the standard generic rules and targets...
include ../build.mk
