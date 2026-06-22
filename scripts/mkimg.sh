#/bin/sh

IMG_FILE=root.img
IMG_SIZE=2048
MOUNT_PATH=/media
ROOTFS_URL=https://dl-cdn.alpinelinux.org/alpine/v3.24/releases/armv7
ROOTFS_FILE=alpine-minirootfs-3.24.1-armv7.tar.gz

dd if=/dev/zero of="${IMG_FILE}" bs=1M count=${IMG_SIZE}

mke2fs -t ext4 -F "${IMG_FILE}"

mount -o loop "${IMG_FILE}" "${MOUNT_PATH}"

cd "${MOUNT_PATH}"

wget "${ROOTFS_URL}/${ROOTFS_FILE}"

tar -xvf "${ROOTFS_FILE}"

