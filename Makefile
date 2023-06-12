PWD := ${CURDIR}
ifneq ($(KERNELRELEASE),)
SRC_DIR := src
EXTRA_CFLAGS := -I$(src)/include
ccflags-y += -g -O3 -Wall -Werror
obj-m += kdriver.o 
kdriver-y := ${SRC_DIR}/driver.o ${SRC_DIR}/file_ops.o

else

KDIR := /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules

.PHONY: clean
clean:
	make -C $(KDIR) M=$(PWD) clean

endif
