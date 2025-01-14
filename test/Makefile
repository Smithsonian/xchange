# Use the parent directories for libraries and headers.
INC = ../include
BUILD_MODE = debug

# Load the common Makefile definitions...
include ../config.mk

.PHONY: all
all: tests run

.PHONY: tests
tests: $(BIN)/test-struct $(BIN)/test-lookup $(BIN)/test-json

.PHONY: run
run: tests
	$(BIN)/test-struct
	$(BIN)/test-lookup
	$(BIN)/test-json

$(BIN)/test-%: $(OBJ)/test-%.o $(LIB)/libxchange.a
	$(MAKE) $(BIN)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean-test
clean-test:
	rm -rf bin

clean: clean-test

# Finally, the standard generic rules and targets...
include ../build.mk

