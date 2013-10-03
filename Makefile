all:
	make -C xcrun/
	make -C xcode-select/

install:
	make -C xcrun/ PREFIX=$(PREFIX) install
	make -C xcode-select/ PREFIX=$(PREFIX) install

.PHONY: clean
clean:
	make -C xcrun/ clean
	make -C xcode-select/ clean
