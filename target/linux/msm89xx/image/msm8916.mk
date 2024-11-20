# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2020-2022 HandsomeMod Project
#

ifeq ($(SUBTARGET),msm8916)

define Device/msm8916
  $(Device/Qcom)
  SOC := msm8916
  ROOT_BLKDEV := "/dev/mmcblk0p14"
  QCOM_CMDLINE := "earlycon console=tty0 console=ttyMSM0,115200 root=/dev/mmcblk0p14 rw rootwait"
  QCOM_BOOTIMG_FLASH_OFFSET_BASE := 0x80000000
  QCOM_BOOTIMG_FLASH_OFFSET_KERNEL := 0x00080000
  QCOM_BOOTIMG_FLASH_OFFSET_RAMDISK := 0x02000000
  QCOM_BOOTIMG_FLASH_OFFSET_SECOND := 0x00f00000
  QCOM_BOOTIMG_FLASH_OFFSET_TAGS := 0x01e00000
  QCOM_BOOTIMG_FLASH_OFFSET_PAGESIZE := 2048
endef

#define Device/XiaoMi_wingtech-wt88047-modem
#  $(Device/msm8916)
#  DEVICE_VENDOR := XiaoMi
#  DEVICE_MODEL := Redmi 2
#  DEVICE_PACKAGES := kmod-qcom-drm kmod-qcom-msm8916-panel kmod-sound-qcom-msm8916 kmod-qcom-modem qcom-msm8916-wt8x047-wcnss-firmware qcom-msm8916-wcnss-wt88047-nv qcom-msm8916-modem-wt88047-firmware 
#endef

#TARGET_DEVICES += XiaoMi_wingtech-wt88047-modem

#define Device/openstick_uz801
#  $(Device/msm8916)
#  DEVICE_VENDOR := Openstick
#  DEVICE_MODEL := OpenStick UZ801
#  DEVICE_PACKAGES := openstick-tweaks wpad-basic-wolfssl kmod-qcom-modem qcom-msm8916-modem-openstick-uz801-firmware qcom-msm8916-openstick-uz801-wcnss-firmware qcom-msm8916-wcnss-openstick-uz801-nv
#endef

# TARGET_DEVICES += openstick_uz801

define Device/msm8916-uz801-v32
  $(Device/msm8916)
  DEVICE_VENDOR := Openstick
  DEVICE_MODEL := OpenStick UZ801 V3.2
  DEVICE_PACKAGES := openstick-tweaks wpad-basic-wolfssl kmod-qcom-rproc-modem \
  qcom-msm8916-modem-openstick-uz801-v3_2-firmware qcom-msm8916-openstick-uz801-v3_2-wcnss-firmware \
  qcom-msm8916-wcnss-openstick-uz801-v3_2-nv
  SUPPORTED_DEVICES += yiming,uz801-v3
endef
TARGET_DEVICES += msm8916-uz801-v32

#define Device/openstick_uf896
#  $(Device/msm8916)
#  DEVICE_VENDOR := Openstick
#  DEVICE_MODEL := UF896
#  DEVICE_PACKAGES := openstick-tweaks wpad-basic-wolfssl kmod-qcom-modem qcom-msm8916-modem-uf896-firmware qcom-msm8916-uf896-wcnss-firmware qcom-msm8916-wcnss-uf896-nv losetup
#endef

# TARGET_DEVICES += openstick_uf896

endif
