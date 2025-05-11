KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

obj-m     := modules_demo.o

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
