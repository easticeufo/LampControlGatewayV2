#!/bin/bash

#用于调试时生成coredump文件
ulimit -c unlimited
echo "/media/mmcblk0p1/core" > /proc/sys/kernel/core_pattern #将coredump文件输出到TF卡中

export TZ=CST-8

#配置网络指示灯NET_LED的GPIO引脚
echo 68 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio68/direction # GPIO设置为输出
echo 1 > /sys/class/gpio/gpio68/value # 关灯

#配置运行指示灯RUN引脚
echo none > /sys/class/leds/led-run/trigger # 设置为用户控制
echo 0 > /sys/class/leds/led-run/brightness # 关灯

WORK_DIR="/opt"

cp LampControlGateway ${WORK_DIR}
cp appweb.conf ${WORK_DIR}
cp shell_cmd ${WORK_DIR}
rm -rf ${WORK_DIR}/webfile
tar -xf webfile.tar -C ${WORK_DIR}

cd ${WORK_DIR}

wr ln -s $(pwd)/shell_cmd /bin/prtHardInfo
wr ln -s $(pwd)/shell_cmd /bin/setDebugLevel
wr ln -s $(pwd)/shell_cmd /bin/prtWebsocketInfo
wr ln -s $(pwd)/shell_cmd /bin/prtLoginInfo
wr ln -s $(pwd)/shell_cmd /bin/prtManufactureTime

echo "run LampControlGateway!"
./LampControlGateway &
