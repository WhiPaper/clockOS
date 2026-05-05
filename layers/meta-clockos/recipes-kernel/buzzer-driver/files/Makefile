obj-m := buzzer_drv.o

SRC := $(shell pwd)

# Device Tree compiler – Yocto provides dtc via DEPENDS in the recipe
DTC ?= dtc

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules
	$(DTC) -@ -I dts -O dtb -o $(SRC)/buzzer.dtbo $(SRC)/buzzer-overlay.dts

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) clean
	rm -f $(SRC)/buzzer.dtbo
