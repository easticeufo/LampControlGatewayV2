ʹ��U-Boot����tftp�����ļ�ϵͳ�ķ�����
1.���ذ���Ҫ�������ߺʹ��ڣ����Һ�PC������ͬһ���������£���PC���ϰ�װtftp������������ʹ��tftpd��
2.���������ļ�ϵͳrootfs.ubifs�ŵ�tftp�������ĸ�Ŀ¼�£���������tftp������
3.�������ذ�ĸ�λ���������ϵ磬��������ʾ��Hit any key to stop autoboot %d��ʱ��������ո��������U-Boot����������ʾ��ģʽ
4.ͨ����������U-Boot�еĻ�������ipaddrΪ���ذ�IP��ַ��serveripΪPC����IP��ַ����Ҫ��֤���ǵ�IP��ͬһ�����Σ������ն��������£�
setenv ipaddr 192.168.31.209
setenv server 192.168.31.211
saveenv
5.�����ն�����run uprootfs����ʼ�����ļ�ϵͳ
6.��������ɺ��ڴ����ն�����reset��������

rootfs.ubifs�ļ�ϵͳ����������
1.ʹ��"sudo tar -xjvf rootfs.tar.bz2"�����ѹrootfs.tar.bz2����ѹ����ļ�ϵͳ��rootfs��
2.����Ӧ�ó���rootfs\home\userappĿ¼��
3.ʹ��"sudo ./build_rootfs.sh"��������build_rootfs.sh�ű����õ���rootfs.ubifs�ļ���������U-Boot�����ļ�ϵͳ
