MOD = aprf_drv
KPATH :=/lib/modules/$(shell uname -r)/build
obj-m := $(MOD).o

all:
	$(MAKE) -C $(KPATH) M=$(shell pwd) modules

clean:
	$(MAKE) -C $(KPATH) M=$(shell pwd) clean
