all: module/niagara.ko user_api/util

# You need following feature only if you have 10G card AND you need LFD timers
# In this case, unpack somewhere Intel IXGBE sources and put path to them here

IXGBE_DIR := $(CURDIR)/ixgbe
IXGBE_SRC_DIR := $(IXGBE_DIR)/src

EXTRA_CFLAGS := $(CFLAGS)
ifneq (, $(wildcard $(IXGBE_SRC_DIR)/ixgbe.h))
	EXTRA_CFLAGS += -DCONFIG_IXGBE_INTERACTION
	EXTRA_CFLAGS += -I$(IXGBE_SRC_DIR)
endif
EXTRA_CFLAGS += -Wall
EXTRA_CFLAGS += -Werror

export EXTRA_CFLAGS

module/niagara.ko: include/niagara_config.h
ifeq (, $(wildcard $(IXGBE_SRC_DIR)/ixgbe.h))
	@echo " *******************"
	@echo " Please download and unpack ixgbe-3.18.7 driver sources to"
	@echo " ixgbe folder in order to get LFD support on 10G NICs"
	@echo " (i.e. X540-based NICs)"
	@echo " *******************"
endif
	make -j -C module modules
user_api/util:
	make -j -C user_api util

insmod: module/niagara.ko
	sudo insmod $<
rmmod:
	sudo rmmod niagara

install: 
#	make -C module modules_install
	make -C user_api install
	sudo install -m 0644 udev/* /lib/udev/rules.d
uninstall:
#	rm -rf /lib/modules/`uname -r`/niagara.ko
	make -C user_api uninstall
clean:
	make -C module   clean
	make -C user_api clean
tar: clean
	cd .. ; tar -cvf universal-driver.0.7b1.tar --exclude='.git/*' $(shell basename ${CURDIR})
