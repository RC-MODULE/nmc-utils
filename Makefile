.SUFFIXES:
-include blackjack.mk


LIBEASYNMC_VERSION=0.1.1
PREFIX?=/usr/
DESTDIR?=
STATIC?=

# Check for NMC compiler and fill in naive defaults
# This is required for container build, since jenkins runs debuild and
# debuild gets rid of all enviroment variables
NEURO?=/opt/module-nmc
NMCC=$(shell which nmcc)
ifeq ($(NMCC),)
export PATH:=$(PATH):$(NEURO)/bin-lnx/
endif

#Handle case when we're not cross-compiling
ifneq ($(GNU_TARGET_NAME),)
CROSS_COMPILE?=$(GNU_TARGET_NAME)-
endif

# If you want to cross-compile against a debian sysroot, you may want to set sysroot
# accordingly. The sysroot should have all the -dev packages required
# The magic below hooks up toolchain's sysroot for pkg-config

SYSROOT:=$(shell dirname `which $(CROSS_COMPILE)gcc`)/../$(GNU_TARGET_NAME)/sysroot/
CFLAGS+=-Iinclude/ -Wall --sysroot=$(SYSROOT)
LDFLAGS+=-Wl,--as-needed --sysroot=$(SYSROOT)

export PKG_CONFIG_DIR=
export PKG_CONFIG_LIBDIR=${SYSROOT}/usr/lib/pkgconfig:${SYSROOT}/usr/share/pkgconfig
export PKG_CONFIG_SYSROOT_DIR=${SYSROOT}
export NEURO

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
lib$(1).so.$(2): $$($(1)-objs)
	$$(SILENT_LD)$$(CROSS_COMPILE)gcc -o $$(@) -shared -fPIC $$(^) $$(LDFLAGS) \
	-Wl,-soname,$$(@)  $$(libs-LDFLAGS)

lib$(1).a: $$($(1)-objs)
	$$(SILENT_AR)$$(CROSS_COMPILE)ar rcs $$(@) $$(^) 

all+=lib$(1).so.$(2) lib$(1).a
all-libs+=lib$(1).so.$(2) lib$(1).a
endef

#unit-tests are always static
define UNIT_TEST_RULE
unit-$(1): unit-tests/$(1).o $$(easynmc-objs)
	$$(SILENT_LD)$$(CROSS_COMPILE)gcc -o $$(@) $$(^) $$(LDFLAGS) -static
all+=$(1)
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
Libs: -L$${libdir} -l:libeasynmc.so.$(LIBEASYNMC_VERSION)
Cflags: -I$${includedir}
endef
export PC_FILE_TEMPLATE

utils+=nmctl nmrun
libs +=easynmc

unit-tests+=appdata

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
utils-LDFLAGS+=-L. -l:libeasynmc.so.$(LIBEASYNMC_VERSION)
endif

all: $(utils) ipl libeasynmc-nmc easynmc-$(LIBEASYNMC_VERSION).pc examples
	@echo > /dev/null

$(foreach l,\
	$(libs),\
	$(eval $(call LIB_RULE,$(l),$(LIBEASYNMC_VERSION))))

$(foreach u,\
	$(utils),\
	$(eval $(call UTIL_RULE,$(u))))

$(foreach u,\
	$(unit-tests),\
	$(eval $(call UNIT_TEST_RULE,$(u))))

easynmc-$(LIBEASYNMC_VERSION).pc: 
	$(SILENT_PKGCONFIG)echo "$$PC_FILE_TEMPLATE" > $(@)

libeasynmc-nmc:
	cd libeasynmc-nmc && $(MAKE) 

unit-tests: $(addprefix unit-,$(unit-tests))

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
	@$(foreach u,$(all-libs),install -D $(u) $(DESTDIR)/$(PREFIX)/lib/$(GNU_TARGET_NAME)/$(u);)
	@$(foreach u,$(shell ls ipl/*.abs),install -D $(u) $(DESTDIR)/$(PREFIX)/share/easynmc/$(u);)

install-dev: all
	@$(foreach u,$(shell ls include/ | grep -v linux),install -D include/$(u)\
		$(DESTDIR)/$(PREFIX)/include/$(GNU_TARGET_NAME)/easynmc-$(LIBEASYNMC_VERSION)/$(u);)
	@install -D include/linux/easynmc.h \
		$(DESTDIR)/$(PREFIX)/include/$(GNU_TARGET_NAME)/easynmc-$(LIBEASYNMC_VERSION)/linux/easynmc.h
	@install -D easynmc-$(LIBEASYNMC_VERSION).pc \
		$(DESTDIR)/$(PREFIX)/lib/$(GNU_TARGET_NAME)/pkgconfig/easynmc-$(LIBEASYNMC_VERSION).pc

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

doxygen: 
	( cat nmc-utils.doxyfile ; echo "PROJECT_NUMBER=$(LIBEASYNMC_VERSION)" ) | doxygen -

include docker.mk

.PHONY: ipl examples libeasynmc-nmc arch-check doxygen \
	install install-bin install-dev install-ipl install-doc install-abs \
	deb unit-tests

