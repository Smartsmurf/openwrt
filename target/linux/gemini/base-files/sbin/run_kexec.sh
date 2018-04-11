#!/bin/sh
# kexec -d -l /tmp/openwrt-gemini-raidsonic-nas4220-zImage --atags --command-line="console=ttyS0,19200 root=/dev/sda1 rootfstype=ext2,squashfs noinitrd" 
kexec -d -l /tmp/openwrt-gemini-raidsonic-nas4220-zImage --dtb=/tmp/gemini-nas4220b.dtb --initrd=/tmp/openwrt-gemini-raidsonic-rootfs.cpio.gz --image-size=12455829 --command-line="console=ttyS0,19200n8 root=/dev/ram0"
echo waiting 20 sec...
sleep 20
kexec -e

