.SUFFIXES:
-include blackjack.mk


LIBEASYNMC_VERSION=0.1.1
PREFIX?=/usr/local/
DESTDIR?=
STATIC?=

# HACK!
# Uncomment this and set to rcm's linux-3.x/include/uapi path if the toolchain
# you are using lacks required headers.
# Do this at your own risk or if you're hacking around with kernel part
#CFLAGS+=-I/home/necromant/work/linux-3.10.x/include/uapi

#Handle case when we're not cross-compiling
ifneq ($(GNU_TARGET_NAME),)
CROSS_COMPILE?=$(GNU_TARGET_NAME)-
endif

# If you want to cross-compile against a debian sysroot, you may want to set sysroot
# accordingly. The sysroot should have all the -dev packages required
# The magic below hooks up toolchain's sysroot for pkg-config

SYSROOT:=$(shell dirname `which $(CROSS_COMPILE)gcc`)/../$(GNU_TARGET_NAME)/sysroot/
CFLAGS+=-Iinclude/ -Wall 

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
CFLAGS+=-DLIBEASYNMC_VERSION=\"$(LIBEASYNMC_VERSION)\"

ifneq ($(STATIC),y)
utils-LDFLAGS+=-L. -leasynmc-$(LIBEASYNMC_VERSION)
endif

all: $(utils) ipl libeasynmc-nmc easynmc-$(LIBEASYNMC_VERSION).pc examples
	@echo > /dev/null

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
	@echo "Installation to $(tb_ylw)$(DESTDIR)/$(PREFIX)$(col_rst) completed!"

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
	@$(foreach u,$(shell find nmc-examples/ -iname "*.abs"),install -D $(u) \
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
	-rm -Rf debroot-*
	-rm *.deb

emc:
	emc *.c include/*.h

arch-check:
	@[ ! -z "$(ARCH)" ] || (echo "Please set ARCH to target debian architecture, e.g. armhf"; exit 1)

bin-deps=nmc-utils-ipl (>=$(LIBEASYNMC_VERSION)), libelf1
dev-deps=nmc-utils-bin (>=$(LIBEASYNMC_VERSION))
abs-deps=nmc-utils-bin (>=$(LIBEASYNMC_VERSION))
doc-deps=nmc-utils-bin (>=$(LIBEASYNMC_VERSION))

deb: deb-bin deb-dev deb-doc deb-ipl deb-abs
	echo "Installation completed!"

deb-%: arch-check
	@mkdir -p debroot-$(*)/DEBIAN/
	@$(MAKE) DESTDIR=./debroot-$(*) PREFIX=/usr install-$(*)
	@./generate-deb-info.sh $(ARCH) $(*) $(LIBEASYNMC_VERSION) \
		'$($(*)-deps)' > debroot-$(*)/DEBIAN/control 
	$(SILENT_DEB)fakeroot dpkg-deb --build debroot-$(*) \
		./nmc-utils-$(*)_$(LIBEASYNMC_VERSION)_$(ARCH).deb
	@rm -Rf debroot-$(*)

.PHONY: ipl examples libeasynmc-nmc arch-check \
	install install-bin install-dev install-ipl install-doc install-abs\
	deb
