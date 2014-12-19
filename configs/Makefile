TARGET ?= DarwinARM
DEVELOPER_DIR ?= /opt/Developer

all:
	@echo "Nothing to do for all"

install: all
	install -d $(DESTDIR)/etc
	install -d $(DESTDIR)/$(DEVELOPER_DIR)/SDKs/$(TARGET).sdk
	install -d $(DESTDIR)/$(DEVELOPER_DIR)/Toolchains/$(TARGET).toolchain
	install -m 644 xcrun.ini $(DESTDIR)/etc/xcrun.ini
	install -m 644 $(TARGET)SDKSettings.info.ini $(DESTDIR)/$(DEVELOPER_DIR)/SDKs/$(TARGET).sdk/info.ini
	install -m 644 $(TARGET)ToolchainSettings.info.ini $(DESTDIR)/$(DEVELOPER_DIR)/Toolchains/$(TARGET).toolchain/info.ini

clean:
	@echo "Nothing to do for clean"
