obj-m += slm_netlink.o

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
