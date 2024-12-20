Package/ar3k-firmware = $(call Package/firmware-default,ath3k firmware,,LICENSE.QualcommAtheros_ar3k)
define Package/ar3k-firmware/install
	$(INSTALL_DIR) $(1)/lib/firmware/ar3k
	$(CP) \
		$(PKG_BUILD_DIR)/ar3k/*.dfu \
		$(1)/lib/firmware/ar3k
	$(INSTALL_DIR) $(1)/lib/firmware/qca
	$(CP) \
		$(PKG_BUILD_DIR)/qca/*.bin \
		$(1)/lib/firmware/qca
endef
$(eval $(call BuildPackage,ar3k-firmware))


Package/ath6k-firmware = $(call Package/firmware-default,AR600X firmware)
define Package/ath6k-firmware/install
	$(INSTALL_DIR) $(1)/lib/firmware/ath6k
	$(CP) \
		$(PKG_BUILD_DIR)/ath6k/* \
		$(1)/lib/firmware/ath6k
endef
$(eval $(call BuildPackage,ath6k-firmware))

Package/ath9k-htc-firmware = $(call Package/firmware-default,AR9271/AR7010 firmware,,LICENCE.open-ath9k-htc-firmware)
define Package/ath9k-htc-firmware/install
	$(INSTALL_DIR) $(1)/lib/firmware/ath9k_htc
	$(INSTALL_DATA) \
		$(PKG_BUILD_DIR)/ath9k_htc/htc_9271-1.4.0.fw \
		$(PKG_BUILD_DIR)/ath9k_htc/htc_7010-1.4.0.fw \
		$(1)/lib/firmware/ath9k_htc
endef
$(eval $(call BuildPackage,ath9k-htc-firmware))

Package/carl9170-firmware = $(call Package/firmware-default,AR9170 firmware,,carl9170fw/GPL)
define Package/carl9170-firmware/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/carl9170-1.fw $(1)/lib/firmware
endef
$(eval $(call BuildPackage,carl9170-firmware))

Package/wil6210-firmware = $(call Package/firmware-default,wil6210 firmware)
define Package/wil6210-firmware/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/wil6210.fw $(1)/lib/firmware
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/wil6210.brd $(1)/lib/firmware
endef
$(eval $(call BuildPackage,wil6210-firmware))

Package/venus-firmware = $(call Package/firmware-default,venus firmware)
define Package/venus-firmware/install
	$(INSTALL_DIR) $(1)/lib/firmware/qcom/venus-1.8
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/qcom/venus-1.8/venus.* $(1)/lib/firmware/qcom/venus-1.8
endef
$(eval $(call BuildPackage,venus-firmware))

Package/adreno-3xx-firmware = $(call Package/firmware-default,adreno a3xx gpu firmware)
define Package/adreno-3xx-firmware/install
	$(INSTALL_DIR) $(1)/lib/firmware/qcom/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/qcom/a300* $(1)/lib/firmware/qcom
endef
$(eval $(call BuildPackage,adreno-3xx-firmware))