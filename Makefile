PREFIX ?= /usr
DEVELOPER_DIR ?= /opt/Developer

TARGET ?= DarwinARM

.PHONY: all install clean

all:
	make -C xcrun/
	make -C xcode-select/

install:
	install -d $(DSTROOT)/$(DEVELOPER_DIR)/SDKs/$(TARGET).sdk
	install -d $(DSTROOT)/$(DEVELOPER_DIR)/Toolchains/$(TARGET).toolchain/usr/bin
	install -m 644 configs/xcrun.ini $(DSTROOT)/etc/xcrun.ini
	install -m 644 configs/$(TARGET)SDKSettings.info.ini $(DSTROOT)/$(DEVELOPER_DIR)/SDKs/$(TARGET).sdk/info.ini
	install -m 644 configs/$(TARGET)ToolchainSettings.info.ini $(DSTROOT)/$(DEVELOPER_DIR)/Toolchains/$(TARGET).toolchain/info.ini
	make -C xcrun/ PREFIX=$(PREFIX) DSTROOT=$(DSTROOT) install
	make -C xcode-select/ PREFIX=$(PREFIX) DSTROOT=$(DSTROOT) install
	install -m 755 scripts/xcrun-tool.sh $(DSTROOT)/$(PREFIX)/bin/xcrun-tool
	install -m 755 scripts/clang.sh $(DSTROOT)/$(DEVELOPER_DIR)/Toolchains/$(TARGET).toolchain/usr/bin/clang
	-ln -s $(DSTROOT)/$(DEVELOPER_DIR)/Toolchains/$(TARGET).toolchain/usr/bin/clang $(DSTROOT)/$(DEVELOPER_DIR)/Toolchains/$(TARGET).toolchain/usr/bin/clang++

ifndef DSTROOT
	xcode-select --switch $(DEVELOPER_DIR)
else
	@echo "Remember to run xcode-select --switch /path/to/DeveloperDir after installation!"
endif

clean:
	make -C xcrun/ clean
	make -C xcode-select/ clean
