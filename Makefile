obj-m := \
	slm_netlink.o

slm_netlink-objs := \
	slm_netlink_main.o   \
	slm_netlink_socket.o \
	slm_netlink_kprobe.o \
	slm_netlink_procfs.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f slm_user

slm_user: slm_user.c Makefile
	$(CC) -o slm_user -ggdb -O0 slm_user.c

.PHONY: all clean
