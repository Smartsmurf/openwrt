#!/bin/sh
kexec -d -l /tmp/openwrt-gemini-raidsonic-nas4220-zImage \
 --dtb=/tmp/gemini-nas4220b.dtb --image-size=12582912

if [ -f "/tmp/openwrt-gemini-raidsonic-rootfs.cpio.gz" ]; then
  kexec -d --initrd=/tmp/openwrt-gemini-raidsonic-rootfs.cpio.gz \
   --command-line="console=ttyS0,19200n8 root=/dev/ram0"
fi

echo waiting 20 sec...
sleep 20
kexec -e

