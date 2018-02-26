#!/bin/bash

./mkfs.ubifs -r rootfs  -m 2048 -e 126976 -c 512 -o ubifs.img
mv -f ubifs.img rootfs.ubifs
