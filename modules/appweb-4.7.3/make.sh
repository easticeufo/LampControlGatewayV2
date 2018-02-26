#!/bin/bash

source ../../compile_config

if [ -z ${TOOL_PREFIX} ]; then
	BUILD_ARCH=$(uname -m | sed 's/i.86/x86/;s/x86_64/x64/;s/arm.*/arm/;s/mips.*/mips/')
else
	BUILD_ARCH=arm
fi

BIN_DIR=build/linux-${BUILD_ARCH}-static/bin

if [ -z $1 ]; then
	make SHOW=1 CC=${TOOL_PREFIX}gcc LD=${TOOL_PREFIX}ld ARCH=${BUILD_ARCH} ME_COM_OPENSSL=0 ME_COM_CGI=0 ME_COM_ESP=0 -f projects/appweb-linux-static.mk
	cp ${BIN_DIR}/libappweb.a ../../libs/
	cp ${BIN_DIR}/libmpr.a ../../libs/
	cp ${BIN_DIR}/libhttp.a ../../libs/
	cp ${BIN_DIR}/libpcre.a ../../libs/
elif [ $1 = "clean" ]; then
	rm -rf build/
else
	echo "command error!"
fi
