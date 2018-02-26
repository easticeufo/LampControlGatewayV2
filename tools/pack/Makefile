
ROOT_DIR = ../..

CC := gcc

INCLUDES := 
INCLUDES += -I$(ROOT_DIR)/include

CFLAGS += -pipe -Wall -O2 $(INCLUDES)

all:build/pack build/unpack

build/pack:build/pack.o build/base_fun.o
	$(CC) -o $@ $^ -lpthread
	cp build/pack $(ROOT_DIR)/package/
	
build/unpack:build/unpack.o build/base_fun.o
	$(CC) -o $@ $^ -lpthread
	cp build/unpack $(ROOT_DIR)/package/

build/pack.o:pack.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	
build/unpack.o:unpack.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	
build/base_fun.o:$(ROOT_DIR)/src/base_fun.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -rf build
