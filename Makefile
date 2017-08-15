MYY_KERNEL_DIR ?= ../RockMyy/linux
ARCH ?= arm
CROSS_COMPILE ?= armv7a-hardfloat-linux-gnueabi-

# If you need to setup specific CFLAGS,
# uncomment the next line and set them there.
# ccflags-y += 

myy-bt-gpio-rfkill-objs := bt-gpio-rfkill.o

# Careful, obj-y is for elements built into the kernel !
# obj-m is for elements built as modules
# We're building a module, so obj-m is required !
obj-m += myy-bt-gpio-rfkill.o

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) -C $(MYY_KERNEL_DIR) modules

clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) -C $(MYY_KERNEL_DIR) clean

install:
	scp myy-vpu.ko 10.100.0.55:/tmp
