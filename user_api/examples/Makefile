all: daemon_kick modules

daemon_kick: daemon_kick.c
	cc -I../../include -Wall -o $@ $< ../api_fs.c
modules clean: module_kick.c
	make -C /lib/modules/`uname -r`/build/ M=${CURDIR} KBUILD_EXTRA_SYMBOLS=${CURDIR}/../../module/Module.symvers $@
insmod:
	sudo insmod ./kick.ko card=0 segment=1
rmmod:
	sudo rmmod kick

obj-m:=kick.o
kick-objs:=module_kick.o

hostprog-y := daemon_kick
ccflags-y:=-I$(src)/../../include
always := $(hostprog-y)
