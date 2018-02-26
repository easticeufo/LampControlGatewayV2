使用U-Boot进行tftp更新文件系统的方法：
1.网关板需要连接网线和串口，并且和PC机放在同一个局域网下，在PC机上安装tftp服务器（建议使用tftpd）
2.将制作的文件系统rootfs.ubifs放到tftp服务器的根目录下，并且启动tftp服务器
3.按下网关板的复位键或重新上电，当串口显示“Hit any key to stop autoboot %d”时立即输入空格键，进入U-Boot的命令行提示符模式
4.通过串口设置U-Boot中的环境变量ipaddr为网关板IP地址、serverip为PC机的IP地址，需要保证他们的IP在同一个网段，串口终端命令如下：
setenv ipaddr 192.168.31.209
setenv server 192.168.31.211
saveenv
5.串口终端输入run uprootfs即开始更新文件系统
6.当升级完成后在串口终端输入reset重启即可

rootfs.ubifs文件系统制作方法：
1.使用"sudo tar -xjvf rootfs.tar.bz2"命令解压rootfs.tar.bz2，解压后的文件系统在rootfs中
2.拷贝应用程序到rootfs\home\userapp目录下
3.使用"sudo ./build_rootfs.sh"命令运行build_rootfs.sh脚本，得到的rootfs.ubifs文件即可用于U-Boot更新文件系统
