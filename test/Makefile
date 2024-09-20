# Use the parent directories for libraries and headers.
LIB = ../lib
INC = ../include

# Load the common Makefile definitions...
include ../config.mk

CFLAGS += -g

export CFLAGS

all: $(BIN)/test-struct $(BIN)/test-lookup $(BIN)/test-json

$(BIN)/test-%: $(OBJ)/test-%.o $(LIB)/libxchange.a | $(BIN)
	$(CC) -o $@ $^ $(LDFLAGS)

$(LIB)/libxchange.a: ../src ../include ../Makefile
	make -C .. static

$(BIN):
	mkdir $(BIN)

.PHONY: clean-test
clean-test:
	rm -rf bin
	
clean: clean-test

# Finally, the standard generic rules and targets...
include ../build.mk
