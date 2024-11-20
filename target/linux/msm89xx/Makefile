# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2020-2022 HandsomeMod Project
#
# THANKS TO:
# Postmarket OS Project
# msm8916-mainline Project

include $(TOPDIR)/rules.mk

ARCH:=aarch64
BOARD:=msm89xx
BOARDNAME:=Qualcomm Msm89xx Soc
FEATURES:=fpu usb display rtc usbgadget gpio squashfs dt virtio
SUBTARGETS:=msm8916
MAINTAINER:=SmartSmurf <smartsmurf@shared-files.de>

KERNEL_PATCHVER:=6.6
KERNEL_TESTING_PATCHVER:=6.6
KERNELNAME:=Image.gz dtbs

define Target/Description
	Build firmware images for Qualcomm Msm89xx Soc based SmartPhones Tablets and Boards.
endef

include $(INCLUDE_DIR)/target.mk

DEFAULT_PACKAGES += partx-utils mkf2fs e2fsprogs kmod-wcn36xx
DEFAULT_PACKAGES += kmod-qcom-rproc kmod-qcom-rproc-wcnss kmod-qcom-bluetooth
DEFAULT_PACKAGES += kmod-usb-chipidea kmod-usb-gadget-eth

$(eval $(call BuildTarget))
