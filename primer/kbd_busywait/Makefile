ifneq ($(KERNELRELEASE),) 
	obj-m := kbd_bw_driver.o
else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build 

PWD := $(shell pwd)

default: 
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules 
	gcc -o test_busywait kbd_bw_test.c kbd_bw_getchar.c
endif 
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

