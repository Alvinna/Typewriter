#!/bin/sh

mount -o loop /mnt/onboard/.adds/typewriter/root.img /mnt/user

mount -v --bind /dev /mnt/user/dev
mount -v --bind /dev/pts /mnt/user/dev/pts
mount -vt proc proc /mnt/user/proc
mount -vt sysfs sysfs /mnt/user/sys
mount -vt tmpfs tmpfs /mnt/user/run
mkdir /mnt/user/dev/shm
mount -vt tmpfs tmpfs /mnt/user/dev/shm
cp /etc/resolv.conf /mnt/user/etc/

chroot /mnt/user /bin/sh

