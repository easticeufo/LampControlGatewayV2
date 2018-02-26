
ROOT_DIR = ../..

include $(ROOT_DIR)/compile_config

CC := $(TOOL_PREFIX)gcc

INCLUDES := 
INCLUDES += -I$(ROOT_DIR)/include

CFLAGS += -pipe -Wall -O2 $(INCLUDES)

all:build/shell_cmd

build/shell_cmd:build/shell_cmd.o build/base_fun.o
	$(CC) -o $@ $^ -lpthread
	cp build/shell_cmd $(ROOT_DIR)/package/

build/shell_cmd.o:shell_cmd.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/base_fun.o:$(ROOT_DIR)/src/base_fun.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -rf build
