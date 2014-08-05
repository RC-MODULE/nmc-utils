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
	@echo "const DEBUG=1; " > conf.mlb
	$(MAKE) TARGET=ipl-k1879-debug clean all
	@echo "const DEBUG=0; " > conf.mlb
	$(MAKE) TARGET=ipl-k1879 clean all
endif

%.asmx: %.cpp
	$(SILENT_DEP)gcc -E -MM $(<) -o$(@).dep
	$(SILENT_NMCPP)$(CROSS_COMPILE)nmcpp $(NMCPP_FLAGS) $(<) -O$(@) $(IDIRS) 

%.o: %.asmx
	$(SILENT_ASM)$(CROSS_COMPILE)asm $(C2ASM_FLAGS) $(<) -o$(@)

%.o: %.asm
	$(SILENT_DEP)gcc -E -MM -xassembler-with-cpp $(<) -o$(@).dep
	$(SILENT_ASM)$(CROSS_COMPILE)nmcc $(ASM_FLAGS) $(<) -o$(@)

$(TARGET).abs: $(OBJECTS)
	-$(SILENT_LINKER)$(CROSS_COMPILE)linker  $(BUILDER_FLAGS) -o$(@) $(^) $(LIBS) $(LIBDIR)

$(TARGET).dump: $(TARGET).abs
	-$(SILENT_NMDUMP)$(CROSS_COMPILE)nmdump -f $(^) > $(@)

run: 
	edcltool -f run_nmc_code.edcl -i eth1

clean:
	-$(SILENT_CLEAN)rm -f *.asmx; rm -f *.o; rm -f *.dump *.dep \
	*.ac *.map *~


