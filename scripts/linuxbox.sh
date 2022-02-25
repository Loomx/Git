#!/bin/sh
#
# Script for minimal self-contained Linux/Busybox EFI binary
# based on https://github.com/ivandavidov/minimal-linux-script/blob/master/minimal.sh
# and Slackware's huge kernel config

KERNEL=5.15.19
BUSYBOX=1.35.0

set -xe

wget -c https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-$KERNEL.tar.xz
wget -c https://mirrors.slackware.com/slackware/slackware64-15.0/source/k/kernel-configs/config-huge-$KERNEL.x64
wget -c https://busybox.net/downloads/busybox-$BUSYBOX.tar.bz2

tar xf busybox-$BUSYBOX.tar.bz2
cd busybox-$BUSYBOX
make defconfig
sed -i 's/.*CONFIG_STATIC .*/CONFIG_STATIC=y/' .config
make busybox install
rm _install/linuxrc
cat > _install/init <<EOF
#!/bin/sh

dmesg -n 1
mkdir -p /dev /proc /sys
mount -t devtmpfs dev /dev
mount -t proc proc /proc
mount -t sysfs sys /sys
while true; do
	setsid cttyhack /bin/sh
done
EOF
chmod +x _install/init
cd ..

tar xf linux-$KERNEL.tar.xz
cd linux-$KERNEL
cp ../config-huge-$KERNEL.x64 .config
sed -i 's/.*INITRAMFS.*/CONFIG_INITRAMFS_SOURCE="_install"/' .config 
make olddefconfig
cp -a ../busybox-$BUSYBOX/_install/ .
make -j7 bzImage
cp arch/x86/boot/bzImage ../bootx64.efi
cd ..

set +xe
