TARGET ?= DarwinARM
DEVELOPER_DIR ?= /opt/Developer

all:
	@echo "Nothing to do for all"

install: all
	install -d $(DESTDIR)/usr/bin
	install -d $(DESTDIR)/$(DEVELOPER_DIR)/SDKs/$(TARGET).sdk/usr/bin
	install -d $(DESTDIR)/$(DEVELOPER_DIR)/Toolchains/$(TARGET).toolchain/usr/bin
	install -m 755 xcrun-tool.sh $(DESTDIR)/usr/bin/xcrun-tool
	install -m 755 cc.sh $(DESTDIR)/$(DEVELOPER_DIR)/Toolchains/$(TARGET).toolchain/usr/bin/cc
	install -m 755 cpp.sh $(DESTDIR)/$(DEVELOPER_DIR)/Toolchains/$(TARGET).toolchain/usr/bin/cpp
	install -m 755 c++.sh $(DESTDIR)/$(DEVELOPER_DIR)/Toolchains/$(TARGET).toolchain/usr/bin/c++
	install -m 755 clang.sh $(DESTDIR)/$(DEVELOPER_DIR)/Toolchains/$(TARGET).toolchain/usr/bin/clang
	-ln -s $(DEVELOPER_DIR)/Toolchains/$(TARGET).toolchain/usr/bin/clang $(DESTDIR)/$(DEVELOPER_DIR)/Toolchains/$(TARGET).toolchain/usr/bin/clang++

clean:
	@echo "Nothing to do for clean"
