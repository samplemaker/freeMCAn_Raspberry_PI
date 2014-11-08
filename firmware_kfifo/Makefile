EXTRA_CFLAGS := -I$(src)/../include

obj-m += firmware_geiger_ts.o

firmware_geiger_ts-objs := kernel_firmware.o

CFLAGS_kernel_firmware.o := -Wno-declaration-after-statement

KVERSION := $(shell uname -r)
KDIR  := /lib/modules/$(KVERSION)/build
PWD   := $(shell pwd)

all:	firmware

firmware:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

