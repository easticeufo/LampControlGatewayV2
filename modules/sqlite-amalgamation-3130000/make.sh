#!/bin/bash

source ../../compile_config

if [ -z $1 ]; then
	${TOOL_PREFIX}gcc -pipe -Wall -O2 -c sqlite3.c
	${TOOL_PREFIX}ar rv libsqlite3.a sqlite3.o
	cp libsqlite3.a ../../libs/
elif [ $1 = "clean" ]; then
	rm -f *.o
	rm -f *.a
else
	echo "command error!"
fi
