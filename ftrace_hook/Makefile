KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

obj-m += ftrace_hook_demo.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
