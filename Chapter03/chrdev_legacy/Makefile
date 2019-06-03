ifndef KERNEL_DIR
$(error KERNEL_DIR must be set in the command line)
endif
PWD := $(shell pwd)
ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-

obj-m = chrdev_legacy.o

all: modules

modules clean:
	make -C $(KERNEL_DIR) \
            ARCH=$(ARCH) \
            CROSS_COMPILE=$(CROSS_COMPILE) \
            SUBDIRS=$(PWD) $@
