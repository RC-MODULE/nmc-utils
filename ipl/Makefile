#
# This is a basic Makefile template for a Neuromatrix DSP project 
# to be run on Module MB77.07. To compile it you need: 
# * Latest NMSDK installed with utilities in your $PATH
# * NEURO environment variable pointing to NMSDK directory
# * Host GCC (Since nmcpp doesn't support generating deps, 
#	       we use gcc for that) 
#
# For verbose build run 'make VERBOSE=y'
#

-include colorizer.mk
-include *.dep

.SUFFIXES:

OBJECTS :=  \
	k1879_init.o \
	k1879_load.o 

TARGET=
CROSS_COMPILE=
NMCPP_FLAGS     = -DNEURO -OPT2 -inline -pie
ASM_FLAGS       = -soc -Sc -Stmp -m -Snoasmpp -O2
C2ASM_FLAGS     = -soc -q
LIBS            = libint_soc.lib cppnew05.lib libc05.lib
BUILDER_FLAGS	= -ck1879_init.cfg 
IDIRS           = -I. -I"$(NEURO)/include" 
LIBDIR		= -l"$(NEURO)/lib" 

.DEFAULT_GOAL=all


ifneq ($(TARGET),)
all: $(TARGET).dump 
else
all: 
	@echo -e "macro DECLARE_DEBUG()\nconst DEBUG=1;\nend DECLARE_DEBUG;" > conf.mlb
	$(MAKE) TARGET=ipl-K1879-nmc-debug clean all
	@echo -e "macro DECLARE_DEBUG()\nconst DEBUG=0;\nend DECLARE_DEBUG;" > conf.mlb
	$(MAKE) TARGET=ipl-K1879-nmc clean all
endif

%.asmx: %.cpp
	$(SILENT_DEP)$(CROSS_COMPILE)gcc -E -MM $(<) -o$(@).dep
	$(SILENT_NMCPP)nmcpp $(NMCPP_FLAGS) $(<) -O$(@) $(IDIRS) 

%.o: %.asmx
	$(SILENT_ASM)asm $(C2ASM_FLAGS) $(<) -o$(@)

%.o: %.asm
	$(SILENT_DEP)$(CROSS_COMPILE)gcc -E -MM -xassembler-with-cpp $(<) -o$(@).dep
	$(SILENT_ASM)nmcc $(ASM_FLAGS) $(<) -o$(@)

$(TARGET).abs: $(OBJECTS)
	-$(SILENT_LINKER)linker  $(BUILDER_FLAGS) -o$(@) $(^) $(LIBS) $(LIBDIR)

$(TARGET).dump: $(TARGET).abs
	-$(SILENT_NMDUMP)nmdump -f $(^) > $(@)

run: 
	edcltool -f run_nmc_code.edcl -i eth1

clean:
	-$(SILENT_CLEAN)rm -f *.asmx; rm -f *.o; rm -f *.dump *.dep \
	*.ac *.map *~


