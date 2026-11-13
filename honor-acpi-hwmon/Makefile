ifneq ($(KBUILD_EXTMOD),)
obj-m := honor_acpi_hwmon.o
else
KDIR ?= /lib/modules/$(shell uname -r)/build
# Match each installed kernel's toolchain (CachyOS uses Clang/ThinLTO).
ifneq ($(shell sed -n 's/^CONFIG_CC_IS_CLANG=y/y/p' $(KDIR)/.config),)
export LLVM := 1
endif

.PHONY: all clean
all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) W=1 modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
endif
