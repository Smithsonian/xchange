INC = ../include

include ../config.mk

CFLAGS += -g

.PHONY: all
all: bin/test-json

bin/test-json: test-json.c ../lib/libxchange.a

../lib/libxchange.a: 
	@make -C .. static
	

bin/%: | bin
	$(CC) -o $@ $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS)

include ../build.mk
