# Load the common Makefile definitions...
include $(GLOBALINC)/setup.mk

IFLAGS += -I../include
CFLAGS0 += -g

all: $(BIN)/test-json

$(BIN)/test-json: $(OBJ)/test-json.o $(XJSON) $(XSTRUCT) $(XCHANGE)


# Finally, the standard generic rules and targets...
include $(GLOBALINC)/recipes.mk
