-include blackjack.mk

.SUFFIXES:

SYSROOT:=$(shell dirname `which $(CROSS_COMPILE)gcc`)/../$(GNU_TARGET_NAME)/sysroot/

define PKG_CONFIG
#CFLAGS  += $$(shell pkg-config --cflags  $(1))
LDFLAGS += $$(shell pkg-config --libs $(1))
endef


define UTIL_RULE
$(1): $$($(1)-objs)
	$$(SILENT_LD)$$(CROSS_COMPILE)gcc -o $$(@) $$(^) $$(LDFLAGS) 
all+=$(1)
endef


$(eval $(call PKG_CONFIG,libelf))

CFLAGS+=-Iinclude/ -Wall -I$(SYSROOT)/usr/include/libelf
CFLAGS+=-I/home/necromant/work/linux-3.10.x/include/uapi
LDFLAGS+=-static

utils+=nmctl
nmctl-objs:=nmctl.o easynmc-core.o


$(foreach u,\
	$(utils),\
	$(eval $(call UTIL_RULE,$(u))))




all: $(all) startupcode

startupcode:
	cd startup_code && $(MAKE) 

.PHONY: startupcode

%.o: %.c 
	$(SILENT_CC)$(CROSS_COMPILE)gcc $(CFLAGS) -c -o $(@) $(<)

libauracore.so: $(obj-y)
	$(SILENT_LD)$(CROSS_COMPILE)gcc -lusb-1.0 -O -shared -fpic -o $(@) $(^) $(LDFLAGS) 

clean: 
	-rm -f *.o *.so $(utils)
	-find . -iname "*~" -delete
	cd startup_code && $(MAKE) clean
emc:
	emc *.c include/*.h



upload: nmctl startupcode
	scp nmctl root@192.168.0.7:
	scp startup_code/startup-k1879.abs root@192.168.0.7:startup.abs


#nmctl: nmctl.c easynmc-core.c
#	$(CROSS_COMPILE)gcc -Iinclude -I/home/necromant/work/linux-3.10.x/include/uapi -static $^ -o $(@)
#	