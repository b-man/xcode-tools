DEVELOPER_DIR ?= /opt/Developer

DIRS := \
	configs \
	scripts \
	xcrun \
	xcode-select

define do_make
	@for dir in $1; do \
		make -C $$dir DESTDIR=$(DESTDIR) DEVELOPER_DIR=$(DEVELOPER_DIR) $2; \
	done
endef

all:
	$(call do_make, $(DIRS), all)

install: all
	$(call do_make, $(DIRS), install)
ifndef DESTDIR
	xcode-select --switch $(DEVELOPER_DIR)
endif

clean:
	$(call do_make, $(DIRS), clean)
