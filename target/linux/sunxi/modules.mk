# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2013-2016 OpenWrt.org

define KernelPackage/mfd-ac100
    SUBMENU:=$(OTHER_MENU)
    TITLE:=X-Powers AC100 MFD support
    DEPENDS:=@TARGET_sunxi
    KCONFIG:= \
	CONFIG_MFD_AC100
    FILES:=$(LINUX_DIR)/drivers/mfd/ac100.ko
    AUTOLOAD:=$(call AutoLoad,50,ac100)
endef

define KernelPackage/mfd-ac100/description
 Support for the X-Powers AC100 RTC/audio chip
endef

$(eval $(call KernelPackage,mfd-ac100))

define KernelPackage/rtc-ac100
    SUBMENU:=$(OTHER_MENU)
    TITLE:=X-Powers AC100 RTC support
    DEPENDS:=@TARGET_sunxi +kmod-mfd-ac100
    $(call AddDepends/rtc)
    KCONFIG:= \
	CONFIG_RTC_DRV_AC100 \
	CONFIG_RTC_CLASS=y
    FILES:=$(LINUX_DIR)/drivers/rtc/rtc-ac100.ko
    AUTOLOAD:=$(call AutoLoad,50,rtc-ac100)
endef

define KernelPackage/rtc-ac100/description
 Support for the X-Powers AC100 RTC
endef

$(eval $(call KernelPackage,rtc-ac100))

define KernelPackage/rtc-sunxi
    SUBMENU:=$(OTHER_MENU)
    TITLE:=Sunxi SoC built-in RTC support
    DEPENDS:=@(TARGET_sunxi&&RTC_SUPPORT)
    KCONFIG:= \
	CONFIG_RTC_DRV_SUNXI \
	CONFIG_RTC_CLASS=y
    FILES:=$(LINUX_DIR)/drivers/rtc/rtc-sunxi.ko
    AUTOLOAD:=$(call AutoLoad,50,rtc-sunxi)
endef

define KernelPackage/rtc-sunxi/description
 Support for the AllWinner sunXi SoC's onboard RTC
endef

$(eval $(call KernelPackage,rtc-sunxi))

define KernelPackage/sunxi-ir
    SUBMENU:=$(OTHER_MENU)
    TITLE:=Sunxi SoC built-in IR support
    DEPENDS:=@(TARGET_sunxi&&RTC_SUPPORT) +kmod-input-core
    KCONFIG:= \
	CONFIG_MEDIA_SUPPORT=y \
	CONFIG_MEDIA_RC_SUPPORT=y \
	CONFIG_RC_DEVICES=y \
	CONFIG_RC_CORE=y \
	CONFIG_IR_SUNXI
    FILES:=$(LINUX_DIR)/drivers/media/rc/sunxi-cir.ko
    AUTOLOAD:=$(call AutoLoad,50,sunxi-cir)
endef

define KernelPackage/sunxi-ir/description
 Support for the AllWinner sunXi SoC's onboard IR
endef

$(eval $(call KernelPackage,sunxi-ir))

define KernelPackage/ata-sunxi
    TITLE:=AllWinner sunXi AHCI SATA support
    SUBMENU:=$(BLOCK_MENU)
    DEPENDS:=@TARGET_sunxi +kmod-ata-ahci-platform +kmod-scsi-core
    KCONFIG:=CONFIG_AHCI_SUNXI
    FILES:=$(LINUX_DIR)/drivers/ata/ahci_sunxi.ko
    AUTOLOAD:=$(call AutoLoad,41,ahci_sunxi,1)
endef

define KernelPackage/ata-sunxi/description
 SATA support for the AllWinner sunXi SoC's onboard AHCI SATA
endef

$(eval $(call KernelPackage,ata-sunxi))

define KernelPackage/sun4i-emac
  SUBMENU:=$(NETWORK_DEVICES_MENU)
  TITLE:=AllWinner EMAC Ethernet support
  DEPENDS:=@TARGET_sunxi +kmod-of-mdio +kmod-libphy
  KCONFIG:=CONFIG_SUN4I_EMAC
  FILES:=$(LINUX_DIR)/drivers/net/ethernet/allwinner/sun4i-emac.ko
  AUTOLOAD:=$(call AutoProbe,sun4i-emac)
endef

$(eval $(call KernelPackage,sun4i-emac))

define KernelPackage/sound-soc-sunxi
  TITLE:=AllWinner built-in SoC sound support
  KCONFIG:=CONFIG_SND_SUN4I_CODEC
  FILES:=$(LINUX_DIR)/sound/soc/sunxi/sun4i-codec.ko
  AUTOLOAD:=$(call AutoLoad,65,sun4i-codec)
  DEPENDS:=@TARGET_sunxi +kmod-sound-soc-core
  $(call AddDepends/sound)
endef

define KernelPackage/sound-soc-sunxi/description
  Kernel support for AllWinner built-in SoC audio
endef

$(eval $(call KernelPackage,sound-soc-sunxi))

define KernelPackage/sound-soc-sunxi-spdif
  TITLE:=Allwinner A10 SPDIF Support
  KCONFIG:=CONFIG_SND_SUN4I_SPDIF
  FILES:=$(LINUX_DIR)/sound/soc/sunxi/sun4i-spdif.ko
  AUTOLOAD:=$(call AutoLoad,65,sun4i-spdif)
  DEPENDS:=@TARGET_sunxi +kmod-sound-soc-spdif
  $(call AddDepends/sound)
endef

define KernelPackage/sound-soc-sunxi-spdif/description
  Kernel support for Allwinner A10 SPDIF Support
endef

$(eval $(call KernelPackage,sound-soc-sunxi-spdif))

define KernelPackage/sun4i-drm
    SUBMENU:=$(DISPLAY_MENU)
    TITLE:=DRM Support for Allwinner A10 Display Engine
    DEPENDS:=@TARGET_sunxi +kmod-backlight +kmod-drm +kmod-drm-kms-helper +kmod-lib-crc-ccitt
    KCONFIG:= \
	CONFIG_ARCH_SUNXI=y \
        	CONFIG_CMA=y \
        	CONFIG_DMA_CMA=y \
	CONFIG_DRM_FBDEV_EMULATION=y \
	CONFIG_DRM_FBDEV_OVERALLOC=100 \
	CONFIG_DRM_SUN4I \
	CONFIG_DRM_SUN4I_HDMI \
	CONFIG_DRM_SUN4I_HDMI_CEC=n \
	CONFIG_DRM_SUN4I_BACKEND \
	CONFIG_DRM_SUN8I_DW_HDMI \
	CONFIG_DRM_SUN6I_DSI \
	CONFIG_DRM_SUN8I_MIXER \
	CONFIG_DRM_PANEL_SIMPLE \
	CONFIG_DRM_PANEL=y \
	CONFIG_DRM_GEM_CMA_HELPER=y \
	CONFIG_DRM_KMS_CMA_HELPER=y \
	CONFIG_RESET_CONTROLLER=y \
	CONFIG_DRM_PANEL_SAMSUNG_LD9040=n \
	CONFIG_DRM_PANEL_SAMSUNG_S6E8AA0=n \
	CONFIG_DRM_PANEL_LG_LG4573=n \
	CONFIG_DRM_PANEL_LD9040=n \
	CONFIG_DRM_PANEL_LVDS=n \
	CONFIG_DRM_PANEL_S6E8AA0=n \
	CONFIG_DRM_PANEL_SITRONIX_ST7789V=n

    FILES:= \
	$(LINUX_DIR)/drivers/gpu/drm/sun4i/sun4i-drm.ko \
	$(LINUX_DIR)/drivers/gpu/drm/sun4i/sun4i-backend.ko \
 	$(LINUX_DIR)/drivers/gpu/drm/sun4i/sun4i-frontend.ko \
	$(LINUX_DIR)/drivers/gpu/drm/sun4i/sun4i-tcon.ko \
	$(LINUX_DIR)/drivers/gpu/drm/sun4i/sun4i_tv.ko \
	$(LINUX_DIR)/drivers/gpu/drm/sun4i/sun6i_mipi_dsi.ko \
	$(LINUX_DIR)/drivers/gpu/drm/sun4i/sun6i_drc.ko \
	$(LINUX_DIR)/drivers/gpu/drm/sun4i/sun8i-mixer.ko \
 	$(LINUX_DIR)/drivers/gpu/drm/sun4i/sun8i_tcon_top.ko \
 	$(LINUX_DIR)/drivers/gpu/drm/panel/panel-simple.ko
    AUTOLOAD:=$(call AutoLoad,08,sun4i-drm sun4i-backend sun4i-frontend sun4i-tcon sun4i_tv sun6i_mipi_dsi sun6i_drc sun8i-mixer sun8i_tcon_top panel-simple)
endef

define KernelPackage/sun4i-drm/description
  DRM Support for Allwinner A10 Display Engine
endef

$(eval $(call KernelPackage,sun4i-drm))

define KernelPackage/sunxi-musb
  SUBMENU:=$(USB_MENU)
  TITLE:=AllWinner family built-in SoC musb controller support
  KCONFIG:= CONFIG_USB_MUSB_SUNXI
  FILES:=$(LINUX_DIR)/drivers/usb/musb/sunxi.ko
  AUTOLOAD:=$(call AutoLoad,53,sunxi)
  DEPENDS:=@TARGET_sunxi +kmod-musb-core
endef

define KernelPackage/sunxi-musb/description
  AllWinner family built-in SoC musb controller support 
endef

$(eval $(call KernelPackage,sunxi-musb))

define KernelPackage/sun6i-csi
  SUBMENU:=$(VIDEO_MENU)
  TITLE:=Allwinner sun6i family Camera Sensor Interface driver
  KCONFIG:= CONFIG_VIDEO_SUN6I_CSI
  FILES:=$(LINUX_DIR)/drivers/media/platform/sunxi/sun6i-csi/sun6i-csi.ko
  AUTOLOAD:=$(call AutoProbe,sun6i-csi)
  DEPENDS:=@TARGET_sunxi_cortexa7 @LINUX_5_4 +kmod-video-csi-core +kmod-video-videobuf2 +kmod-video-videobuf2-dma-contig
endef

define KernelPackage/sun6i-csi/description
  Allwinner sun6i family Camera Sensor Interface driver
endef

$(eval $(call KernelPackage,sun6i-csi))

define KernelPackage/sun4i-csi
  SUBMENU:=$(VIDEO_MENU)
  TITLE:=Allwinner sun4i family Camera Sensor Interface driver
  KCONFIG:= CONFIG_VIDEO_SUN4I_CSI
  FILES:=$(LINUX_DIR)/drivers/media/platform/sunxi/sun4i-csi/sun4i-csi.ko
  AUTOLOAD:=$(call AutoProbe,sun4i-csi)
  DEPENDS:=@TARGET_sunxi @LINUX_5_4 +kmod-video-csi-core +kmod-video-videobuf2 +kmod-video-videobuf2-dma-contig
endef

define KernelPackage/sun4i-csi/description
  Allwinner sun4i family Camera Sensor Interface driver
endef

$(eval $(call KernelPackage,sun4i-csi))

# nonfree vpu driver
define KernelPackage/sunxi-cedarx
  SUBMENU:=Video Encoder/Decoder Support
  TITLE:=Allwinner VPU encoder/decoder module
  DEPENDS:=@TARGET_sunxi @LINUX_5_4
  KCONFIG:= CONFIG_GENERIC_ALLOCATOR=y \
         CONFIG_STAGING_MEDIA=y \
         CONFIG_CMA_DEBUG=n \
         CONFIG_CMA_DEBUGFS=n \
         CONFIG_VIDEO_SUNXI=y \
         CONFIG_DMA_SHARED_BUFFER=y \
         CONFIG_VIDEO_SUNXI_CEDAR_ION=y \
         CONFIG_VIDEO_SUNXI_CEDAR_VE \
         CONFIG_CMA=y \
         CONFIG_DMA_CMA=y \
         CONFIG_CMA_SIZE_MBYTES=16 \
         CONFIG_CMA_SIZE_SEL_MBYTES=y \
         CONFIG_CMA_ALIGNMENT=8 \
         CONFIG_CMA_AREAS=7
  FILES:= $(LINUX_DIR)/drivers/staging/media/sunxi/cedar/ve/cedar_ve.ko
  AUTOLOAD:=$(call AutoProbe,cedar_ve)
endef
$(eval $(call KernelPackage,sunxi-cedarx))

# open-source vpu driver
define KernelPackage/sunxi-cedrus
  SUBMENU:=Video Encoder/Decoder Support
  TITLE:=Allwinner Open-Source VPU Encoder/Decoder module
  DEPENDS:=@TARGET_sunxi @LINUX_5_4 +kmod-video-core +kmod-video-videobuf2 +kmod-video-videobuf2-dma-contig
  KCONFIG:= CONFIG_GENERIC_ALLOCATOR=y \
         CONFIG_STAGING_MEDIA=y \
         CONFIG_CMA_DEBUG=n \
         CONFIG_CMA_DEBUGFS=n \
         CONFIG_VIDEO_SUNXI=y \
         CONFIG_DMA_SHARED_BUFFER=y \
         CONFIG_VIDEO_SUNXI_CEDRUS=y \
         CONFIG_V4L_MEM2MEM_DRIVERS=y \
         CONFIG_VIDEO_MEM2MEM_DEINTERLACE=n \
         CONFIG_VIDEO_SH_VEU=n \
         CONFIG_CMA=y \
         CONFIG_DMA_CMA=y \
         CONFIG_CMA_SIZE_MBYTES=16 \
         CONFIG_CMA_SIZE_SEL_MBYTES=y \
         CONFIG_CMA_ALIGNMENT=8 \
         CONFIG_CMA_AREAS=7
endef
$(eval $(call KernelPackage,sunxi-cedrus))