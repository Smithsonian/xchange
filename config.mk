# ===========================================================================
# Generic configuration options for building the xchange library (both static 
# and shared).
#
# You can include this snipplet in your Makefile also.
# ============================================================================

# Folders in which sources and header files are located, respectively
SRC ?= src
INC ?= include

# Folders for compiled objects, libraries, and binaries, respectively 
OBJ ?= obj
LIB ?= lib
BIN ?= bin

# Compiler: use gcc by default
CC ?= gcc

# Add include/ directory
CPPFLAGS += -I$(INC)

# Base compiler options (if not defined externally...)
CFLAGS ?= -g -Os -Wall -std=c99

# Extra warnings (not supported on all compilers)
#CFLAGS += -Wextra

# Extra linker flags to use
#LDFLAGS =

# cppcheck options for 'check' target
CHECKOPTS ?= --enable=performance,warning,portability,style --language=c \
            --error-exitcode=1 --inline-suppr --std=c99 $(CHECKEXTRA)


# Exhaustive checking for newer cppcheck
#CHECKOPTS += --check-level=exhaustive

# Specific Doxygen to use if not the default one
#DOXYGEN ?= /opt/bin/doxygen

# ============================================================================
# END of user config section. 
#
# Below are some generated constants based on the one that were set above
# ============================================================================

# Compiler and linker options etc.
ifeq ($(BUILD_MODE),debug)
	CFLAGS += -g -DDEBUG
endif

# Link against math libs (for e.g. isnan())
LDFLAGS += -lm

# Search for files in the designated locations
vpath %.h $(INC)
vpath %.c $(SRC)
vpath %.o $(OBJ)
vpath %.d dep 

