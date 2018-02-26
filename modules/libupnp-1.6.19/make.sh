#!/bin/bash

source ../../compile_config

if [ -z $1 ]; then
	./configure --host=${TOOL_PREFIX%-}
	make
	cp upnp/.libs/libupnp.a ../../libs/
	cp ixml/.libs/libixml.a ../../libs/
	cp threadutil/.libs/libthreadutil.a ../../libs/
elif [ $1 = "clean" ]; then
	make clean
else
	echo "command error!"
fi
