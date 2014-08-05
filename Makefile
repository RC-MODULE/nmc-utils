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

utils+=nmctl nmrun
nmctl-objs:=nmctl.o easynmc-core.o easynmc-filters.o
nmrun-objs:=nmrun.o easynmc-core.o easynmc-filters.o

PREFIX?=/usr
DESTDIR?=./intall-test

all: $(utils) ipl libeasynmc-nmc

$(foreach u,\
	$(utils),\
	$(eval $(call UTIL_RULE,$(u))))



libeasynmc-nmc:
	cd libeasynmc-nmc && $(MAKE) 

examples: libeasynmc-nmc
	@for e in `find nmc-examples/* -maxdepth 1 -type d `; do \
	cd $$e && $(MAKE) && cd ../..; \
	done

examples-clean: 
	@for e in `find nmc-examples/* -maxdepth 1 -type d`; do \
	cd $$e && $(MAKE) clean && cd ../..; \
	done


install: all
	$(foreach u,$(utils),install -D $(u) $(DESTDIR)/$(PREFIX)/bin/$(u);)
	$(foreach u,$(libs),install -D $(u) $(DESTDIR)/$(PREFIX)/lib/$(u);)
	$(foreach u,$(shell ls ipl/*.abs),install -D $(u) $(DESTDIR)/$(PREFIX)/share/easynmc/$(u);)

install-dev: all
	$(foreach u,$(shell ls include/),install -D include/$(u) $(DESTDIR)/$(PREFIX)/include/easynmc/$(u);)

install-doc: all
	$(foreach u,$(shell ls include/),install -D include/$(u) $(DESTDIR)/$(PREFIX)/include/easynmc/$(u);)


ipl:
	cd ipl && $(MAKE) 

%.o: %.c 
	$(SILENT_CC)$(CROSS_COMPILE)gcc $(CFLAGS) -c -o $(@) $(<)

libauracore.so: $(obj-y)
	$(SILENT_LD)$(CROSS_COMPILE)gcc -lusb-1.0 -O -shared -fpic -o $(@) $(^) $(LDFLAGS) 

clean: examples-clean
	-rm -f *.o *.so $(utils)
	-find . -iname "*~" -delete
	cd ipl && $(MAKE) clean
	cd libeasynmc-nmc && $(MAKE) clean
emc:
	emc *.c include/*.h


upload: all
	scp nmctl root@192.168.0.7:
	scp nmrun root@192.168.0.7:
	scp startup_code/startup-k1879.abs root@192.168.0.7:startup.abs
	scp libeasynmc-nmc/*.abs root@192.168.0.7:

.PHONY: ipl examples libeasynmc-nmc 
