# ===============================================================================
# WARNING! You should leave this Makefile alone probably
#          To configure the build, you can edit config.mk, or else you export the 
#          equivalent shell variables prior to invoking 'make' to adjust the
#          build configuration. 
# ===============================================================================

include config.mk

# ===============================================================================
# Specific build targets and recipes below...
# ===============================================================================

# The version of the shared .so libraries
SO_VERSION := 1

# Link against math libs (for e.g. isnan())
LDFLAGS += -lm

# Check if there is a doxygen we can run
ifndef DOXYGEN
  DOXYGEN := $(shell which doxygen)
else
  $(shell test -f $(DOXYGEN))
endif

# If there is doxygen, build the API documentation also by default
ifeq ($(.SHELLSTATUS),0)
  DOC_TARGETS += local-dox
else
  $(info WARNING! Doxygen is not available. Will skip 'dox' target) 
endif

# Build everything...
.PHONY: all
all: shared static $(DOC_TARGETS) check

# Shared libraries (versioned and unversioned)
.PHONY: shared
shared: $(LIB)/libxchange.so

# Legacy static libraries (locally built)
.PHONY: static
static: $(LIB)/libxchange.a

# Run regression tests
.PHONY: test
test:
	make -C test

# Remove intermediates
.PHONY: clean
clean:
	rm -f $(OBJECTS) README-orig.md gmon.out
	make -C test clean

# Remove all generated files
.PHONY: distclean
distclean: clean
	rm -f $(LIB)/libxchange.so* $(LIB)/libxchange.a
	make -C test distclean
	

# ----------------------------------------------------------------------------
# The nitty-gritty stuff below
# ----------------------------------------------------------------------------

SOURCES = xchange.c xstruct.c xjson.c

# Generate a list of object (obj/*.o) files from the input sources
OBJECTS := $(subst $(SRC),$(OBJ),$(SOURCES))
OBJECTS := $(subst .c,.o,$(OBJECTS))

$(LIB)/libxchange.so: $(LIB)/libxchange.so.$(SO_VERSION)

# Shared library
$(LIB)/libxchange.so.$(SO_VERSION): $(SOURCES)

# Static library
$(LIB)/libxchange.a: $(OBJECTS) | $(LIB) Makefile

README-orig.md: README.md
	LINE=`sed -n '/\# /{=;q;}' $<` && tail -n +$$((LINE+2)) $< > $@

dox: README-orig.md

.INTERMEDIATE: Doxyfile.local
Doxyfile.local:
	sed "s:resources/header.html::g" Doxyfile > $@

# Local documentation without specialized headers. The resulting HTML documents do not have
# Google Search or Analytics tracking info.
.PHONY: local-dox
local-dox: README-orig.md Doxyfile.local
	doxygen Doxyfile.local

# Built-in help screen for `make help`
.PHONY: help
help:
	@echo
	@echo "Syntax: make [target]"
	@echo
	@echo "The following targets are available:"
	@echo
	@echo "  shared        Builds the shared 'libsupernovas.so', 'libsolsys1.so', and" 
	@echo "                'libsolsys2.so' libraries (linked to versioned ones)."
	@echo "  static        Builds the static 'lib/libsupernovas.a' library."
	@echo "  local-dox     Compiles local HTML API documentation using 'doxygen'."
	@echo "  check         Performs static analysis with 'cppcheck'."
	@echo "  all           All of the above."
	@echo "  clean         Removes intermediate products."
	@echo "  distclean     Deletes all generated files."
	@echo

# This Makefile depends on the config and build snipplets.
Makefile: config.mk build.mk

# ===============================================================================
# Generic targets and recipes below...
# ===============================================================================

include build.mk

	
