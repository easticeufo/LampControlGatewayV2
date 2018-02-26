#!/bin/bash

#���ڵ���ʱ����coredump�ļ�
ulimit -c unlimited
echo "/media/mmcblk0p1/core" > /proc/sys/kernel/core_pattern #��coredump�ļ������TF����

export TZ=CST-8

#��������ָʾ��NET_LED��GPIO����
echo 68 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio68/direction # GPIO����Ϊ���
echo 1 > /sys/class/gpio/gpio68/value # �ص�

#��������ָʾ��RUN����
echo none > /sys/class/leds/led-run/trigger # ����Ϊ�û�����
echo 0 > /sys/class/leds/led-run/brightness # �ص�

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
