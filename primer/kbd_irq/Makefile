ifneq ($(KERNELRELEASE),) 
	obj-m := kbd_irq_driver.o
else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build 

PWD := $(shell pwd)

default: 
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules 
	gcc -o test_irq kbd_irq_test.c kbd_irq_getchar.c
endif 
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

