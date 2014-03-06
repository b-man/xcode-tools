PREFIX ?= /usr
DEVELOPER_DIR ?= /opt/Developer

TARGET ?= DarwinARM

all:
	make -C xcrun/
	make -C xcode-select/

install:
	install -d $(DEVELOPER_DIR)/SDKs/$(TARGET).sdk
	install -d $(DEVELOPER_DIR)/Toolchains/$(TARGET).toolchain
	install -m 644 configs/xcrun.ini $(DSTROOT)/etc/xcrun.ini
	install -m 644 configs/$(TARGET)SDKSettings.info.ini $(DEVELOPER_DIR)/SDKs/$(TARGET).sdk/info.ini
	install -m 644 configs/$(TARGET)ToolchainSettings.info.ini $(DEVELOPER_DIR)/Toolchains/$(TARGET).toolchain/info.ini
	make -C xcrun/ PREFIX=$(PREFIX) DSTROOT=$(DSTROOT) install
	make -C xcode-select/ PREFIX=$(PREFIX) DSTROOT=$(DSTROOT) install
	install -m 755 scripts/xcrun-tool.sh $(DSTROOT)/$(PREFIX)/bin/xcrun-tool
ifneq ($(PACKAGE_BUILD),YES)
	xcode-select --switch $(DEVELOPER_DIR)
endif

.PHONY: clean
clean:
	make -C xcrun/ clean
	make -C xcode-select/ clean
