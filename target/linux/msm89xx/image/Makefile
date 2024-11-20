# SPDX-License-Identifier: GPL-2.0-only

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk


define Build/Compile
	$(CP) $(LINUX_DIR)/COPYING $(KDIR)/COPYING.linux
endef

### Image scripts ###
#define Build/aboot-img
#	mkbootimg \
#		--kernel $@ \
#		--ramdisk $(if $(CONFIG_TARGET_ROOTFS_INITRAMFS),\
#			$(KERNEL_BUILD_DIR)/initrd.cpio$(strip $(call Build/initrd_compression))\
#			,/dev/null) \
#		--cmdline $(CMDLINE) \
#		-o $@.new
#	mv $@.new $@
#endef
define Build/aboot-img
	mkbootimg \
		--kernel $@ \
		--ramdisk /dev/null \
		--cmdline $(QCOM_CMDLINE) \
		-o $@.new
	mv $@.new $@
endef


define Build/sparse-img
	img2simg  $@ $@.new
	mv $@.new $@
endef

#define Build/append-dtb
#
#endef

### Devices ###
define Device/Default
	PROFILES := Default
	BLOCKSIZE := 512k
	KERNEL := kernel-bin | gzip | append-dtb | aboot-img
	IMAGES := boot.img system.img sysupgrade.bin
	IMAGE/boot.img := append-kernel | append-metadata
    IMAGE/system.img := append-rootfs | pad-rootfs | pad-to $$$${BLOCKSIZE} | append-metadata
	IMAGE/sysupgrade.bin/squashfs := append-rootfs | pad-to $$$${BLOCKSIZE} | sysupgrade-tar | append-metadata
	DEVICE_DTS_DIR := $(DTS_DIR)/qcom
	DEVICE_DTS = $(lastword $(subst _, ,$(1)))
	CMDLINE := "earlycon console=ttyMSM0,115200 "\
		   $(if $(CONFIG_TARGET_ROOTFS_INITRAMFS),,\
			   $(if $(ROOT_BLKDEV),"root=$(ROOT_BLKDEV) rw rootwait"))
endef
# 	IMAGE/sysupgrade.bin/squashfs := append-kernel | pad-to 64k | append-rootfs | pad-to $$$${BLOCKSIZE} | append-metadata
#   IMAGE/sysupgrade.bin/squashfs := append-rootfs | pad-to 64k | sysupgrade-tar kernel=$$$$(KDIR)/tmp/handsomemod-$(firstword $(subst /, ,$(TARGETID)))-$(lastword $(subst /, ,$(TARGETID)))-$(1)-squashfs-boot.img rootfs=$$$$@ | append-metadata
# 	IMAGE/boot.img := append-kernel | aboot-img | append-metadata
#	ifneq (,$(findstring uz801,$(1)))
#		DEVICE_DTS = $$(QCOM_DTS_DIR)#$$(SOC)-uz801-v32
#	else
#		DEVICE_DTS = $$(QCOM_DTS_DIR)$$(SOC)-$(lastword $(subst _, ,$(1)))
#	endif

# 	QCOM_DTS_DIR := $(DTS_DIR)/qcom/
#	DEVICE_DTS_DIR := $(DTS_DIR)/qcom
#	DEVICE_DTS = $$(SOC)-$(lastword $(subst _, ,$(1)))

# include $(SUBTARGET).mk
include msm8916.mk

$(eval $(call BuildImage))
