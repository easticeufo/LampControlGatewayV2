#!/bin/bash

DATE=$(date +%Y%m%d)

#需要打包进firmware的文件
PACK_FILE="LampControlGateway appweb.conf initrun.sh shell_cmd webfile.tar"

./pack firmware${DATE} ${PACK_FILE}
