#
# Copyright (C) 2009 David Cooper <dave@kupesoft.com>
# Copyright (C) 2006-2010 OpenWrt.org
# Copyright (C) 2020-2021 HandsomeMod Project
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

VIDEO_MENU:=Camera Support

V4L2_DIR=v4l2-core
V4L2_USB_DIR=usb

#
# Csi Cameras
#

define KernelPackage/video-csi-core
  MENU:=1
  TITLE:=MIPI/CSI Based Camera core support framework
  DEPENDS:=+kmod-video-core +kmod-i2c-algo-bit
  KCONFIG:= \
	CONFIG_VIDEO_V4L2=y \
  	CONFIG_VIDEO_V4L2_SUBDEV_API=y \
  	CONFIG_MEDIA_CONTROLLER=y \
  	CONFIG_MEDIA_CONTROLLER_REQUEST_API=y \
  	CONFIG_VIDEO_V4L2_I2C=y \
  	CONFIG_VIDEO_TDA1997X=n \
  	CONFIG_VIDEO_ADV748X=n \
  	CONFIG_VIDEO_ADV7604=n \
  	CONFIG_VIDEO_VIDEO_ADV7604=n \
  	CONFIG_VIDEO_MUX=n \
  	CONFIG_VIDEO_ADV7842=n \
  	CONFIG_VIDEO_XILINX=n \
  	CONFIG_VIDEO_TC358743=n \
  	CONFIG_VIDEO_ADV7511=n \
  	CONFIG_VIDEO_AD9389B=n \
  	CONFIG_VIDEO_AD5820=n \
  	CONFIG_VIDEO_AK7375=n \
  	CONFIG_VIDEO_DW9714=n \
  	CONFIG_VIDEO_DW9807_VCM=n \
  	CONFIG_VIDEO_ADP1653=n \
  	CONFIG_VIDEO_LM3560=n \
  	CONFIG_VIDEO_LM3646=n \
  	CONFIG_VIDEO_ST_MIPID02=n \
  	CONFIG_VIDEO_GS1662=n \
  	CONFIG_V4L2_FWNODE 	
  FILES:=$(LINUX_DIR)/drivers/media/$(V4L2_DIR)/v4l2-fwnode.ko \
  $(LINUX_DIR)/drivers/media/$(V4L2_DIR)/v4l2-async.ko
  AUTOLOAD:=$(call AutoProbe,v4l2-fwnode)
  $(call AddDepends/camera)
endef

define KernelPackage/video-csi-core/description
 Kernel modules for supporting MIPI/CSI Based Camera. Note this is just
 the core of the driver, please select a submodule that supports your Camera.
endef

$(eval $(call KernelPackage,video-csi-core))


define AddDepends/camera-csi
  SUBMENU:=$(VIDEO_MENU)
  DEPENDS+=kmod-video-csi-core $(1)
endef

define KernelPackage/video-csi-ov2640
  TITLE:=OmniVision OV2640 sensor support
  KCONFIG:=CONFIG_VIDEO_OV2640
  FILES:=$(LINUX_DIR)/drivers/media/i2c/ov2640.ko
  AUTOLOAD:=$(call AutoProbe,ov2640)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-ov2640/description
 OmniVision OV2640 sensor support
endef

$(eval $(call KernelPackage,video-csi-ov2640))

define KernelPackage/video-csi-ov5640
  TITLE:=OmniVision OV5640 sensor support
  KCONFIG:=CONFIG_VIDEO_OV5640
  FILES:=$(LINUX_DIR)/drivers/media/i2c/ov5640.ko
  AUTOLOAD:=$(call AutoProbe,ov5640)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-ov5640/description
 OmniVision OV5640 sensor support
endef

$(eval $(call KernelPackage,video-csi-ov5640))

define KernelPackage/video-csi-ov2680
  TITLE:=OmniVision OV2680 sensor support
  KCONFIG:=CONFIG_VIDEO_OV2680
  FILES:=$(LINUX_DIR)/drivers/media/i2c/ov2680.ko
  AUTOLOAD:=$(call AutoProbe,ov2680)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-ov2680/description
 OmniVision ov2680 sensor support
endef

$(eval $(call KernelPackage,video-csi-ov2680))

define KernelPackage/video-csi-ov2685
  TITLE:=OmniVision ov2685 sensor support
  KCONFIG:=CONFIG_VIDEO_OV2685
  FILES:=$(LINUX_DIR)/drivers/media/i2c/ov2685.ko
  AUTOLOAD:=$(call AutoProbe,ov2685)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-ov2685/description
 OmniVision ov2685 sensor support
endef

$(eval $(call KernelPackage,video-csi-ov2685))

define KernelPackage/video-csi-ov5645
  TITLE:=OmniVision ov5645 sensor support
  KCONFIG:=CONFIG_VIDEO_OV5645
  FILES:=$(LINUX_DIR)/drivers/media/i2c/ov5645.ko
  AUTOLOAD:=$(call AutoProbe,ov5645)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-ov5645/description
 OmniVision ov5645 sensor support
endef

$(eval $(call KernelPackage,video-csi-ov5645))

define KernelPackage/video-csi-ov5647
  TITLE:=OmniVision ov5647 sensor support
  KCONFIG:=CONFIG_VIDEO_OV5647
  FILES:=$(LINUX_DIR)/drivers/media/i2c/ov5647.ko
  AUTOLOAD:=$(call AutoProbe,ov5647)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-ov5647/description
 OmniVision ov5647 sensor support
endef

$(eval $(call KernelPackage,video-csi-ov5647))

define KernelPackage/video-csi-ov5670
  TITLE:=OmniVision ov5670 sensor support
  KCONFIG:=CONFIG_VIDEO_OV5670
  FILES:=$(LINUX_DIR)/drivers/media/i2c/ov5670.ko
  AUTOLOAD:=$(call AutoProbe,ov5670)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-ov5670/description
 OmniVision ov5670 sensor support
endef

$(eval $(call KernelPackage,video-csi-ov5670))

define KernelPackage/video-csi-ov5675
  TITLE:=OmniVision ov5675 sensor support
  KCONFIG:=CONFIG_VIDEO_OV5675
  FILES:=$(LINUX_DIR)/drivers/media/i2c/ov5675.ko
  AUTOLOAD:=$(call AutoProbe,ov5675)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-ov5675/description
 OmniVision ov5675 sensor support
endef

$(eval $(call KernelPackage,video-csi-ov5675))

define KernelPackage/video-csi-ov7251
  TITLE:=OmniVision ov7251 sensor support
  KCONFIG:=CONFIG_VIDEO_OV7251
  FILES:=$(LINUX_DIR)/drivers/media/i2c/ov7251.ko
  AUTOLOAD:=$(call AutoProbe,ov7251)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-ov7251/description
 OmniVision ov7251 sensor support
endef

$(eval $(call KernelPackage,video-csi-ov7251))

define KernelPackage/video-csi-ov8856
  TITLE:=OmniVision ov8856 sensor support
  KCONFIG:=CONFIG_VIDEO_OV8856
  FILES:=$(LINUX_DIR)/drivers/media/i2c/ov8856.ko
  AUTOLOAD:=$(call AutoProbe,ov8856)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-ov8856/description
 OmniVision ov8856 sensor support
endef

$(eval $(call KernelPackage,video-csi-ov8856))

define KernelPackage/video-csi-ov9650
  TITLE:=OmniVision ov9650 sensor support
  KCONFIG:=CONFIG_VIDEO_OV9650
  FILES:=$(LINUX_DIR)/drivers/media/i2c/ov9650.ko
  AUTOLOAD:=$(call AutoProbe,ov9650)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-ov9650/description
 OmniVision ov9650 sensor support
endef

$(eval $(call KernelPackage,video-csi-ov9650))

define KernelPackage/video-csi-ov13858
  TITLE:=OmniVision ov13858 sensor support
  KCONFIG:=CONFIG_VIDEO_OV13858
  FILES:=$(LINUX_DIR)/drivers/media/i2c/ov13858.ko
  AUTOLOAD:=$(call AutoProbe,ov13858)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-ov13858/description
 OmniVision ov13858 sensor support
endef

$(eval $(call KernelPackage,video-csi-ov13858))

define KernelPackage/video-csi-mt9m001
  TITLE:=mt9m001 sensor support
  KCONFIG:=CONFIG_VIDEO_MT9M001
  FILES:=$(LINUX_DIR)/drivers/media/i2c/mt9m001.ko
  AUTOLOAD:=$(call AutoProbe,mt9m001)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-mt9m001/description
 mt9m001 sensor support
endef

$(eval $(call KernelPackage,video-csi-mt9m001))

define KernelPackage/video-csi-mt9m032
  TITLE:=mt9m032 sensor support
  KCONFIG:=CONFIG_VIDEO_MT9M032
  FILES:=$(LINUX_DIR)/drivers/media/i2c/mt9m032.ko
  AUTOLOAD:=$(call AutoProbe,mt9m032)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-mt9m032/description
 mt9m032 sensor support
endef

$(eval $(call KernelPackage,video-csi-mt9m032))

define KernelPackage/video-csi-mt9p031
  TITLE:=MT9P031 sensor support
  KCONFIG:=CONFIG_VIDEO_MT9M032
  FILES:=$(LINUX_DIR)/drivers/media/i2c/mt9p031.ko
  AUTOLOAD:=$(call AutoProbe,mt9p031)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-mt9p031/description
 mt9p031 sensor support
endef

$(eval $(call KernelPackage,video-csi-mt9p031))

define KernelPackage/video-csi-mt9t001
  TITLE:=MT9T001 sensor support
  KCONFIG:=CONFIG_VIDEO_MT9T001
  FILES:=$(LINUX_DIR)/drivers/media/i2c/mt9t001.ko
  AUTOLOAD:=$(call AutoProbe,mt9t001)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-mt9t001/description
 mt9t001 sensor support
endef

$(eval $(call KernelPackage,video-csi-mt9t001))

define KernelPackage/video-csi-imx214
  TITLE:=Sony imx214 sensor support
  KCONFIG:=CONFIG_VIDEO_IMX214
  FILES:=$(LINUX_DIR)/drivers/media/i2c/imx214.ko
  AUTOLOAD:=$(call AutoProbe,imx214)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-imx214/description
 Sony imx214 sensor support
endef

$(eval $(call KernelPackage,video-csi-imx214))

define KernelPackage/video-csi-imx258
  TITLE:=Sony imx258 sensor support
  KCONFIG:=CONFIG_VIDEO_IMX258
  FILES:=$(LINUX_DIR)/drivers/media/i2c/imx258.ko
  AUTOLOAD:=$(call AutoProbe,imx258)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-imx258/description
 Sony imx258 sensor support
endef

$(eval $(call KernelPackage,video-csi-imx258))

define KernelPackage/video-csi-imx274
  TITLE:=Sony imx274 sensor support
  KCONFIG:=CONFIG_VIDEO_IMX274
  FILES:=$(LINUX_DIR)/drivers/media/i2c/imx274.ko
  AUTOLOAD:=$(call AutoProbe,imx274)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-imx274/description
 Sony imx274 sensor support
endef

$(eval $(call KernelPackage,video-csi-imx274))

define KernelPackage/video-csi-imx319
  TITLE:=Sony imx319 sensor support
  KCONFIG:=CONFIG_VIDEO_IMX319
  FILES:=$(LINUX_DIR)/drivers/media/i2c/imx319.ko
  AUTOLOAD:=$(call AutoProbe,imx319)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-imx319/description
 Sony imx319 sensor support
endef

$(eval $(call KernelPackage,video-csi-imx319))

define KernelPackage/video-csi-imx355
  TITLE:=Sony imx355 sensor support
  KCONFIG:=CONFIG_VIDEO_IMX355
  FILES:=$(LINUX_DIR)/drivers/media/i2c/imx355.ko
  AUTOLOAD:=$(call AutoProbe,imx355)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-imx355/description
 Sony imx355 sensor support
endef

$(eval $(call KernelPackage,video-csi-imx355))

define KernelPackage/video-csi-m5mols
  TITLE:=Fujitsu M-5MOLS 8MP sensor support
  KCONFIG:=CONFIG_VIDEO_M5MOLS
  FILES:=$(LINUX_DIR)/drivers/media/i2c/m5mols.ko
  AUTOLOAD:=$(call AutoProbe,m5mols)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-m5mols/description
 Fujitsu M-5MOLS 8MP sensor support
endef

$(eval $(call KernelPackage,video-csi-m5mols))

define KernelPackage/video-csi-s5k6aa
  TITLE:=Samsung S5K6AAFX sensor support
  KCONFIG:=CONFIG_VIDEO_S5K6AA
  FILES:=$(LINUX_DIR)/drivers/media/i2c/s5k6aa.ko
  AUTOLOAD:=$(call AutoProbe,s5k6aa)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-s5k6aa/description
 Samsung S5K6AAFX sensor support
endef

$(eval $(call KernelPackage,video-csi-s5k6aa))

define KernelPackage/video-csi-s5k6a3
  TITLE:=Samsung S5K6A3 sensor support
  KCONFIG:=CONFIG_VIDEO_S5K6AA
  FILES:=$(LINUX_DIR)/drivers/media/i2c/s5k6a3.ko
  AUTOLOAD:=$(call AutoProbe,s5k6a3)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-s5k6a3/description
 Samsung S5K6A3 sensor support
endef

$(eval $(call KernelPackage,video-csi-s5k6a3))

define KernelPackage/video-csi-s5k4ecgx
  TITLE:=Samsung S5K4ECGX sensor support
  KCONFIG:=CONFIG_VIDEO_S5K4ECGX
  FILES:=$(LINUX_DIR)/drivers/media/i2c/s5k4ecgx.ko
  AUTOLOAD:=$(call AutoProbe,s5k4ecgx)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-s5k4ecgx/description
 Samsung S5K4ECGX sensor support
endef

$(eval $(call KernelPackage,video-csi-s5k4ecgx))


define KernelPackage/video-csi-s5k5baf
  TITLE:=Samsung S5K5BAF sensor support
  KCONFIG:=CONFIG_VIDEO_S5K5BAF
  FILES:=$(LINUX_DIR)/drivers/media/i2c/s5k5baf.ko
  AUTOLOAD:=$(call AutoProbe,s5k5baf)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-s5k5baf/description
 Samsung S5K5BAF sensor support
endef

$(eval $(call KernelPackage,video-csi-s5k5baf))

define KernelPackage/video-csi-simapp
  TITLE:=SMIA++/SMIA sensor support
  KCONFIG:=CONFIG_VIDEO_SMIAPP
  FILES:=$(LINUX_DIR)/drivers/media/i2c/simapp.ko
  AUTOLOAD:=$(call AutoProbe,simapp)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-simapp/description
 SMIA++/SMIA sensor support
endef

$(eval $(call KernelPackage,video-csi-simapp))

define KernelPackage/video-csi-et8ek8
  TITLE:=ET8EK8 camera sensor support
  KCONFIG:=CONFIG_VIDEO_ET8EK8
  FILES:=$(LINUX_DIR)/drivers/media/i2c/et8ek8.ko
  AUTOLOAD:=$(call AutoProbe,et8ek8)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-et8ek8/description
 ET8EK8 camera sensor support
endef

$(eval $(call KernelPackage,video-csi-et8ek8))

define KernelPackage/video-csi-s5c73m3
  TITLE:=Samsung S5C73M3 sensor support
  KCONFIG:=CONFIG_VIDEO_S5C73M3
  FILES:=$(LINUX_DIR)/drivers/media/i2c/s5c73m3.ko
  AUTOLOAD:=$(call AutoProbe,s5c73m3)
  $(call AddDepends/camera-csi)
endef

define KernelPackage/video-csi-s5c73m3/description
 Samsung S5C73M3 sensor support
endef

$(eval $(call KernelPackage,video-csi-s5c73m3))

