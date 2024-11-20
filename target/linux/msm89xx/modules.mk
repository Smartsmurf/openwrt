# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2024 SmartSmurf

define KernelPackage/qcom-bluetooth
    SUBMENU:=$(OTHER_MENU)
    TITLE:=Qualcomm SoC built-in bluetooth support
    DEPENDS:=@TARGET_msm89xx +kmod-bluetooth +kmod-qcom-rproc-wcnss
    KCONFIG:= \
         CONFIG_BT_QCOMSMD \
         CONFIG_BT_HCIUART_QCA=y \
         CONFIG_INPUT_PM8941_PWRKEY=n \
         CONFIG_INPUT_PM8XXX_VIBRATOR=n
    FILES:= $(LINUX_DIR)/drivers/bluetooth/btqcomsmd.ko
    AUTOLOAD:=$(call AutoLoad,50,btqcomsmd)
endef

define KernelPackage/qcom-bluetooth/description
 Support for the Qualcomm SoC's bluetooth
endef

$(eval $(call KernelPackage,qcom-bluetooth))

define KernelPackage/qcom-camss
       SUBMENU:=$(VIDEO_MENU)
       TITLE:=Qualcomm V4L2 Camera Subsystem driver
       DEPENDS:=@TARGET_msm89xx +kmod-video-csi-core +kmod-video-videobuf2
       KCONFIG:= \
              CONFIG_VIDEO_QCOM_CAMSS \
              CONFIG_V4L2_FLASH_LED_CLASS=n \
              CONFIG_I2C_QCOM_CCI
       FILES:= \
              $(LINUX_DIR)/drivers/media/platform/qcom/camss/qcom-camss.ko \
              $(LINUX_DIR)/drivers/i2c/busses/i2c-qcom-cci.ko
       AUTOLOAD:=$(call AutoProbe,i2c-qcom-cci qcom-camss)
endef

define KernelPackage/qcom-camss/description
       Qualcomm V4L2 Camera Subsystem driver
endef

$(eval $(call KernelPackage,qcom-camss))

define KernelPackage/qcom-venus
  SUBMENU:=Video Encoder/Decoder Support
  TITLE:=Qualcomm Venus V4L2 encoder/decoder driver
  DEPENDS:=@TARGET_msm89xx +kmod-video-core +kmod-video-videobuf2 +kmod-video-gspca-core +kmod-qcom-rproc +venus-firmware +kmod-video-videobuf2-dma-contig 
  KCONFIG:= \
         CONFIG_V4L_MEM2MEM_DRIVERS=y \
         CONFIG_VIDEO_MEM2MEM_DEINTERLACE=y \
         CONFIG_MEDIA_CONTROLLER_DVB=n \
         CONFIG_V4L2_MEM2MEM_DEV \
         CONFIG_VIDEO_QCOM_VENUS
  FILES:= \
         $(LINUX_DIR)/drivers/media/platform/qcom/venus/venus-core.ko \
         $(LINUX_DIR)/drivers/media/platform/qcom/venus/venus-dec.ko \
         $(LINUX_DIR)/drivers/media/platform/qcom/venus/venus-enc.ko \
         $(LINUX_DIR)/drivers/media/v4l2-core/v4l2-mem2mem.ko
  AUTOLOAD:=$(call AutoProbe,v4l2-mem2mem venus-core venus-dec venus-enc)
endef
$(eval $(call KernelPackage,qcom-venus))

define KernelPackage/rpmsg-wwan-ctrl
  SUBMENU:=$(NETWORK_DEVICES_MENU)
  TITLE:=RPMSG WWAN Control
  DEPENDS:=@TARGET_msm89xx +kmod-wwan
  KCONFIG:=CONFIG_RPMSG_WWAN_CTRL
  FILES:=$(LINUX_DIR)/drivers/net/wwan/rpmsg_wwan_ctrl.ko
  AUTOLOAD:=$(call AutoProbe,rpmsg_wwan_ctrl)
endef

define KernelPackage/rpmsg-wwan-ctrl/description
 Driver for RPMSG WWAN Control
 This exposes all modem control ports like AT, QMI that use RPMSG
endef

$(eval $(call KernelPackage,rpmsg-wwan-ctrl))

define KernelPackage/bam-demux
  SUBMENU:=$(NETWORK_DEVICES_MENU)
  TITLE:=Qualcomm BAM-DMUX WWAN network driver
  DEPENDS:=@TARGET_msm89xx +kmod-wwan
  KCONFIG:=CONFIG_QCOM_BAM_DMUX
  FILES:=$(LINUX_DIR)/drivers/net/wwan/qcom_bam_dmux.ko
  AUTOLOAD:=$(call AutoProbe,qcom_bam_dmux)
endef

define KernelPackage/bam-demux/description
  Kernel modules for Qualcomm BAM-DMUX WWAN interface
endef

$(eval $(call KernelPackage,bam-demux))

define KernelPackage/qcom-modem
  SUBMENU:=$(NETWORK_DEVICES_MENU)
  TITLE:=Qualcomm modem subsystem support
  DEPENDS:=@TARGET_msm89xx +kmod-qcom-rproc +qrtr +rpmsgexport +rmtfs +kmod-bam-demux
  KCONFIG:= \
         CONFIG_RMNET \
         CONFIG_WWAN \
         CONFIG_QCOM_Q6V5_COMMON \
         CONFIG_QCOM_Q6V5_MSS \
         CONFIG_QCOM_Q6V5_ADSP=n \
         CONFIG_QCOM_Q6V5_PAS=n \
         CONFIG_QCOM_Q6V5_WCSS=n \
         CONFIG_QCOM_IPA=n
  FILES:= \
         $(LINUX_DIR)/drivers/net/ethernet/qualcomm/rmnet/rmnet.ko \
         $(LINUX_DIR)/drivers/remoteproc/qcom_q6v5.ko \
         $(LINUX_DIR)/drivers/remoteproc/qcom_q6v5_mss.ko
  AUTOLOAD:=$(call AutoLoad,50,rmnet qcom_q6v5 qcom_q6v5_mss)
endef

$(eval $(call KernelPackage,qcom-modem))

# Note : 
# (1) need userspace daemon (q6voiced) when kmod-qcom-modem is selected.
# (2) apq8016 is also supported
#
define KernelPackage/sound-qcom-msm8916
  TITLE:=Qualcomm msm8916 built-in SoC sound support
  DEPENDS:=@TARGET_msm89xx +kmod-sound-soc-core +kmod-qcom-rproc
  KCONFIG:= \
         CONFIG_SND_SOC_QCOM \
         CONFIG_SND_SOC_STORM=n \
         CONFIG_SND_SOC_AW8738=n \
         CONFIG_SND_SOC_TFA989X=n \
         CONFIG_SND_SOC_MSM8996=n \
         CONFIG_SND_SOC_APQ8016_SBC \
         CONFIG_SND_SOC_QDSP6_Q6VOICE \
         CONFIG_SND_SOC_MSM8916_QDSP6 \
         CONFIG_SND_SOC_MAX98357A \
         CONFIG_SND_SOC_MSM8916_WCD_ANALOG \
         CONFIG_SND_SOC_MSM8916_WCD_DIGITAL \
         CONFIG_SND_SOC_SIMPLE_AMPLIFIER \
         CONFIG_QCOM_APR
  FILES:= \
         $(LINUX_DIR)/sound/soc/qcom/snd-soc-qcom-common.ko \
         $(LINUX_DIR)/drivers/soc/qcom/apr.ko \
         $(LINUX_DIR)/sound/soc/qcom/snd-soc-apq8016-sbc.ko \
         $(LINUX_DIR)/sound/soc/codecs/snd-soc-msm8916-analog.ko \
         $(LINUX_DIR)/sound/soc/codecs/snd-soc-msm8916-digital.ko \
         $(LINUX_DIR)/sound/soc/codecs/snd-soc-max98357a.ko \
         $(LINUX_DIR)/sound/soc/codecs/snd-soc-simple-amplifier.ko \
         $(LINUX_DIR)/sound/soc/qcom/qdsp6/q6adm.ko \
         $(LINUX_DIR)/sound/soc/qcom/qdsp6/q6afe.ko \
         $(LINUX_DIR)/sound/soc/qcom/qdsp6/q6afe-clocks.ko \
         $(LINUX_DIR)/sound/soc/qcom/qdsp6/q6afe-dai.ko \
         $(LINUX_DIR)/sound/soc/qcom/qdsp6/q6apm-dai.ko \
         $(LINUX_DIR)/sound/soc/qcom/qdsp6/q6apm-lpass-dais.ko \
         $(LINUX_DIR)/sound/soc/qcom/qdsp6/q6asm.ko \
         $(LINUX_DIR)/sound/soc/qcom/qdsp6/q6asm-dai.ko \
         $(LINUX_DIR)/sound/soc/qcom/qdsp6/q6core.ko \
         $(LINUX_DIR)/sound/soc/qcom/qdsp6/q6prm.ko \
         $(LINUX_DIR)/sound/soc/qcom/qdsp6/q6prm-clocks.ko \
         $(LINUX_DIR)/sound/soc/qcom/qdsp6/q6routing.ko \
         $(LINUX_DIR)/sound/soc/qcom/qdsp6/snd-q6apm.ko \
         $(LINUX_DIR)/sound/soc/qcom/qdsp6/snd-q6dsp-common.ko
  AUTOLOAD:=$(call AutoProbe,apr snd-soc-apq8016-sbc snd-soc-qcom-common snd-soc-msm8916-qdsp6 q6voice snd-soc-msm8916-analog snd-soc-msm8916-digital snd-soc-max98357a snd-soc-simple-amplifier)
  $(call AddDepends/sound)
endef

define KernelPackage/sound-qcom-msm8916/description
  Kernel support for Qualcomm msm8916 built-in SoC sound codec
endef

$(eval $(call KernelPackage,sound-qcom-msm8916))

define KernelPackage/qcom-drm
    SUBMENU:=$(VIDEO_MENU)
    TITLE:=DRM & GPU Support for Qualcomm Socs
    DEPENDS:=@TARGET_msm89xx +kmod-backlight +adreno-3xx-firmware +kmod-backlight-pwm +kmod-drm +kmod-drm-ttm +kmod-drm-kms-helper +kmod-lib-crc-ccitt +kmod-qcom-rproc
    KCONFIG:= \
         CONFIG_DRM_PANEL=y \
         CONFIG_DRM_MSM \
         CONFIG_DRM_MSM_HDMI_HDCP=y \
         CONFIG_DRM_MSM_DP=y \
         CONFIG_DRM_MSM_GPU_SUDO=n \
         CONFIG_DRM_MSM_DSI=y \
         CONFIG_DRM_MSM_DSI_PLL=y \
         CONFIG_DRM_MSM_DSI_28NM_PHY=y \
         CONFIG_DRM_MSM_DSI_20NM_PHY=y \
         CONFIG_DRM_MSM_DSI_28NM_8960_PHY=y \
         CONFIG_DRM_MSM_DSI_14NM_PHY=y \
         CONFIG_DRM_MSM_DSI_10NM_PHY=y \
         CONFIG_DRM_MSM_DSI_7NM_PHY=y \
         CONFIG_DRM_MSM_REGISTER_LOGGING=n \
         CONFIG_DRM_PANEL_SIMPLE \
         CONFIG_DRM_DISPLAY_CONNECTOR \
         CONFIG_DRM_LEGACY=y \
         CONFIG_DRM_BRIDGE=y \
         CONFIG_DRM_PANEL_ORIENTATION_QUIRKS=y \
         CONFIG_DRM_PANEL_BRIDGE=y \
         CONFIG_DRM_FBDEV_EMULATION=y \
         CONFIG_DRM_FBDEV_OVERALLOC=100 \
         CONFIG_DRM_GEM_SHMEM_HELPER=y \
         CONFIG_DRM_PANEL_ASUS_Z00T_TM5P5_NT35596=n \
         CONFIG_DRM_PANEL_BOE_HIMAX8279D=n \
         CONFIG_DRM_PANEL_BOE_TV101WUM_NL6=n \
         CONFIG_DRM_PANEL_ELIDA_KD35T133=n \
         CONFIG_DRM_PANEL_FEIXIN_K101_IM2BA02=n \
         CONFIG_DRM_PANEL_LEADTEK_LTK050H3146W=n \
         CONFIG_DRM_PANEL_LEADTEK_LTK500HD1829=n \
         CONFIG_DRM_PANEL_NOVATEK_NT35510=n \
         CONFIG_DRM_PANEL_MANTIX_MLAF057WE51=n \
         CONFIG_DRM_PANEL_SITRONIX_ST7703=n \
         CONFIG_DRM_PANEL_SONY_ACX424AKP=n \
         CONFIG_DRM_PANEL_VISIONOX_RM69299=n \
         CONFIG_DRM_PANEL_XINPENG_XPP055C272=n
    FILES:= \
         $(LINUX_DIR)/drivers/gpu/drm/display/drm_display_helper.ko \
         $(LINUX_DIR)/drivers/gpu/drm/display/drm_dp_aux_bus.ko \
         $(LINUX_DIR)/drivers/gpu/drm/scheduler/gpu-sched.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/panel-simple.ko \
         $(LINUX_DIR)/drivers/gpu/drm/msm/msm.ko \
         $(LINUX_DIR)/drivers/gpu/drm/bridge/display-connector.ko
    AUTOLOAD:=$(call AutoLoad,70,panel-simple display-connector msm)
endef

define KernelPackage/qcom-drm/description
  DRM and GPU Support for Qualcomm Socs.
endef

$(eval $(call KernelPackage,qcom-drm))

# we only load panel for wt88047 now.
define KernelPackage/qcom-msm8916-panel
    SUBMENU:=$(VIDEO_MENU)
    TITLE:=Common Mipi Panel Support for Qualcomm Msm8916
    DEPENDS:=@TARGET_msm89xx +kmod-qcom-drm
    KCONFIG:= CONFIG_DRM_PANEL_MSM8916_GENERATED
    FILES:= \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-alcatel-auo-hx8394d.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-asus-z00l-otm1284a.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-asus-z010d-r69339.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-huawei-tianma-nt35521.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-longcheer-booyi-otm1287.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-longcheer-truly-otm1288a.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-longcheer-yushun-nt35520.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-motorola-harpia-boe.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-motorola-harpia-tianma.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-motorola-osprey-inx.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-motorola-surnia-boe.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-oppo-15009-nt35592-jdi.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-samsung-ea8061v-ams497ee01.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-samsung-s6d7aa0-ltl101at01.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-samsung-s6e88a0-ams427ap24.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-samsung-tc358764-ltl101al06.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-wingtech-auo-r61308.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-wingtech-ebbg-otm1285a.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-wingtech-sharp-r69431.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-wingtech-tianma-hx8394d.ko \
         $(LINUX_DIR)/drivers/gpu/drm/panel/msm8916-generated/panel-wingtech-yassy-ili9881.ko
    AUTOLOAD:=$(call AutoLoad,50,panel-wingtech-auo-r61308 panel-wingtech-ebbg-otm1285a panel-wingtech-sharp-r69431 panel-wingtech-tianma-hx8394d panel-wingtech-yassy-ili9881)
endef

define KernelPackage/qcom-msm8916-panel/description
  Form msm8916-mainline project include some panel in smartphone.
endef

$(eval $(call KernelPackage,qcom-msm8916-panel))

#
#
#
# TravMurav openwrt msm89xx
define KernelPackage/qcom-rproc
  SUBMENU:=$(OTHER_MENU)
  TITLE:=Qualcomm remoteproc support
  DEPENDS:=@TARGET_msm89xx
  KCONFIG:=\
    CONFIG_QCOM_MDT_LOADER \
    CONFIG_QCOM_RPROC_COMMON \
    CONFIG_QCOM_PIL_INFO
  FILES:=\
    $(LINUX_DIR)/drivers/soc/qcom/mdt_loader.ko \
    $(LINUX_DIR)/drivers/remoteproc/qcom_common.ko \
    $(LINUX_DIR)/drivers/remoteproc/qcom_pil_info.ko
  AUTOLOAD:=$(call AutoProbe,mdt_loader qcom_common qcom_pil_info)
endef

define KernelPackage/qcom-rproc/description
Support for loading remoteprocs in some Qualcomm chipsets
endef

$(eval $(call KernelPackage,qcom-rproc))

define KernelPackage/qcom-rproc-wcnss
  SUBMENU:=$(OTHER_MENU)
  TITLE:=Qualcomm WCNSS remoteproc support
  DEPENDS:=@TARGET_msm89xx +kmod-qcom-rproc
  KCONFIG:=\
    CONFIG_QCOM_WCNSS_PIL \
    CONFIG_QCOM_WCNSS_CTRL
  FILES:=\
    $(LINUX_DIR)/drivers/remoteproc/qcom_wcnss_pil.ko \
    $(LINUX_DIR)/drivers/soc/qcom/wcnss_ctrl.ko
  AUTOLOAD:=$(call AutoProbe,qcom_wcnss_pil wcnss_ctrl)
endef

define KernelPackage/qcom-rproc-wcnss/description
Firmware loading and control for the WCNSS remoteproc
endef

$(eval $(call KernelPackage,qcom-rproc-wcnss))

define KernelPackage/qcom-rproc-modem
  SUBMENU:=$(OTHER_MENU)
  TITLE:=Qualcomm modem remoteproc support
  DEPENDS:=@TARGET_msm89xx +kmod-qcom-rproc
  KCONFIG:=\
    CONFIG_QCOM_Q6V5_COMMON \
    CONFIG_QCOM_Q6V5_MSS
  FILES:=\
    $(LINUX_DIR)/drivers/remoteproc/qcom_q6v5.ko \
    $(LINUX_DIR)/drivers/remoteproc/qcom_q6v5_mss.ko
  AUTOLOAD:=$(call AutoProbe,qcom_q6v5 qcom_q6v5_mss)
endef

define KernelPackage/qcom-rproc-modem/description
Firmware loading and control for the modem remoteproc.
endef

$(eval $(call KernelPackage,qcom-rproc-modem))