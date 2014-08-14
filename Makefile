.SUFFIXES:
-include blackjack.mk




LIBEASYNMC_VERSION=1.0
PREFIX?=/usr/local/
DESTDIR?=
STATIC?=

# HACK!
# Uncomment this and set to rcm's linux-3.x/include/uapi path if the toolchain
# you are using lacks required headers.
# Do this at your own risk or if you're hacking around with kernel part
 CFLAGS+=-I/home/necromant/work/linux-3.10.x/include/uapi


#CFLAGS+=-Iinclude/ -Wall -I$(SYSROOT)/usr/include/libelf
SYSROOT:=$(shell dirname `which $(CROSS_COMPILE)gcc`)/../$(GNU_TARGET_NAME)/sysroot/
export PKG_CONFIG_DIR=
export PKG_CONFIG_LIBDIR=${SYSROOT}/usr/lib/pkgconfig:${SYSROOT}/usr/share/pkgconfig
export PKG_CONFIG_SYSROOT_DIR=${SYSROOT}


define PKG_CONFIG
CFLAGS  += $$(shell pkg-config --cflags  $(1))
LDFLAGS += $$(shell pkg-config --libs $(1))
endef

define UTIL_RULE
$(1): $$($(1)-objs) $$(all-libs)
	$$(SILENT_LD)$$(CROSS_COMPILE)gcc -o $$(@) $$($(1)-objs) $$(LDFLAGS) $$(utils-LDFLAGS)
all+=$(1)
endef

define LIB_RULE
lib$(1)-$(2).so: $$($(1)-objs)
	$$(SILENT_LD)$$(CROSS_COMPILE)gcc -o $$(@) -shared -fPIC $$(^) $$(LDFLAGS) $$(libs-LDFLAGS)
all+=lib$(1)-$(2).so
all-libs+=lib$(1)-$(2).so
endef

$(eval $(call PKG_CONFIG,libelf))


define PC_FILE_TEMPLATE
prefix=$(PREFIX)
exec_prefix=$${prefix}
libdir=$${exec_prefix}/lib
includedir=$${prefix}/include/easynmc-$(LIBEASYNMC_VERSION)
sysconfdir=/etc

Name: EasyNMC
Description: libEasyNMC DSP library
Version: $(LIBEASYNMC_VERSION)
Requires: libelf
Libs: -L$${libdir} -leasynmc-$(LIBEASYNMC_VERSION)
Cflags: -I$${includedir}
endef
export PC_FILE_TEMPLATE



utils+=nmctl nmrun
libs +=easynmc

easynmc-objs:=easynmc-core.o easynmc-filters.o
nmctl-objs:=nmctl.o
nmrun-objs:=nmrun.o 

ifeq ($(STATIC),y)
nmctl-objs+=$(easynmc-objs)
nmrun-objs+=$(easynmc-objs)
utils-LDFLAGS+=-static
endif

CFLAGS+=-fPIC
CFLAGS+=-DLIBEASYNMC_VERSION=\"1.0\"

ifneq ($(STATIC),y)
utils-LDFLAGS+=-L. -leasynmc-$(LIBEASYNMC_VERSION)
endif

all: $(utils) ipl libeasynmc-nmc easynmc-$(LIBEASYNMC_VERSION).pc examples

$(foreach l,\
	$(libs),\
	$(eval $(call LIB_RULE,$(l),$(LIBEASYNMC_VERSION))))

$(foreach u,\
	$(utils),\
	$(eval $(call UTIL_RULE,$(u))))



easynmc-$(LIBEASYNMC_VERSION).pc: 
	$(SILENT_PKGCONFIG)echo "$$PC_FILE_TEMPLATE" > $(@)

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

install: install-bin install-dev install-doc install-ipl install-abs
	echo "Installation completed!"

install-bin: all
	@$(foreach u,$(utils),install -D $(u) $(DESTDIR)/$(PREFIX)/bin/$(u);)
	@$(foreach u,$(all-libs),install -D $(u) $(DESTDIR)/$(PREFIX)/lib/$(u);)
	@$(foreach u,$(shell ls ipl/*.abs),install -D $(u) $(DESTDIR)/$(PREFIX)/share/easynmc/$(u);)

install-dev: all
	@$(foreach u,$(shell ls include/),install -D include/$(u)\
		$(DESTDIR)/$(PREFIX)/include/easynmc-$(LIBEASYNMC_VERSION)/$(u);)
	@install -D easynmc-$(LIBEASYNMC_VERSION).pc \
		$(DESTDIR)/$(PREFIX)/lib/pkgconfig/easynmc-$(LIBEASYNMC_VERSION).pc

install-doc: all
	@$(foreach u,$(shell ls doc/),install -D doc/$(u) \
	 $(DESTDIR)/$(PREFIX)/share/doc/easynmc-$(LIBEASYNMC_VERSION)/$(u);)

install-ipl: all
	@$(foreach u,$(shell ls ipl/*.abs),install -D $(u) \
	 $(DESTDIR)/$(PREFIX)/share/easynmc-$(LIBEASYNMC_VERSION)/$(u);)

install-abs: all
	@$(foreach u,$(shell find -iname "*.abs"),install -D $(u) \
	 $(DESTDIR)/$(PREFIX)/share/examples/easynmc-$(LIBEASYNMC_VERSION)/$(shell basename $(u));)


ipl:
	cd ipl && $(MAKE) 

%.o: %.c 
	$(SILENT_CC)$(CROSS_COMPILE)gcc $(CFLAGS) -c -o $(@) $(<)

clean: examples-clean
	-rm -f *.o *.so $(utils) *.pc
	-find . -iname "*~" -delete
	cd ipl && $(MAKE) clean
	cd libeasynmc-nmc && $(MAKE) clean
emc:
	emc *.c include/*.h


upload: all
	scp nmctl root@192.168.0.7:
	scp nmrun root@192.168.0.7:
	scp nmlogd root@192.168.0.7:
	scp ipl/startup-k1879.abs root@192.168.0.7:startup.abs
	scp libeasynmc-nmc/*.abs root@192.168.0.7:

.PHONY: ipl examples libeasynmc-nmc 
