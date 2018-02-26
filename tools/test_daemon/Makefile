
ROOT_DIR = ../..

CC := gcc

INCLUDES := 
INCLUDES += -I$(ROOT_DIR)/include

CFLAGS += -pipe -Wall -O2 $(INCLUDES)

all:build/test_daemon

build/test_daemon:build/test_daemon.o build/base_fun.o
	$(CC) -o $@ $^ -lpthread -lrt
	cp build/test_daemon $(ROOT_DIR)/test/

build/test_daemon.o:test_daemon.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/base_fun.o:$(ROOT_DIR)/src/base_fun.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -rf build
