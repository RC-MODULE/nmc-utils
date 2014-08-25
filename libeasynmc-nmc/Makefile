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
	startup.o \
	printf.o \
	easynmc-io.o \
	platform.o

TARGET=easynmc

CROSS_COMPILE=
NMCPP_FLAGS     = -DNEURO -OPT2 -inline -I./include
ASM_FLAGS       = -soc -Sc -Stmp -X-q -I./include
C2ASM_FLAGS     = -soc -q 
LIBS            = libc05.lib
BUILDER_FLAGS	= -cmb7707brd.cfg -m -heap=0 -heap1=0 -heap2=0 -heap3=0 -stack=20000 \
		  -full_names
IDIRS           = -I. -I"$(NEURO)/include" 
LIBDIR		= -l"$(NEURO)/lib" 

.DEFAULT_GOAL=all

all: $(TARGET).lib

%.asmx: %.cpp
	$(SILENT_DEP)$(CROSS_COMPILE)gcc -E -MM $(<) -o$(@).dep
	$(SILENT_NMCPP)nmcpp -Tp $(NMCPP_FLAGS) $(<) -O$(@) $(IDIRS) 

%.asmx: %.c
	$(SILENT_DEP)$(CROSS_COMPILE)gcc -E -MM $(<) -o$(@).dep
	$(SILENT_NMCPP)nmcpp -Tc99 $(NMCPP_FLAGS) $(<) -O$(@) $(IDIRS) 

%.o: %.asmx
	$(SILENT_ASM)asm $(C2ASM_FLAGS) $(<) -o$(@)

%.o: %.asm
	$(SILENT_DEP)$(CROSS_COMPILE)gcc -E -MM -xassembler-with-cpp $(<) -o$(@).dep
	$(SILENT_ASM)nmcc $(ASM_FLAGS) $(<) -o$(@)

$(TARGET).lib: $(OBJECTS)
	-$(SILENT_LIBRARIAN)libr -c $(@) $(^) > /dev/null

$(TARGET).abs: $(OBJECTS)
	-$(SILENT_LINKER)linker  $(BUILDER_FLAGS) -o$(@) $(^) $(LIBS) $(LIBDIR)

$(TARGET).dump: $(TARGET).abs
	-$(SILENT_NMDUMP)nmdump -f $(^) > $(@)

run: $(TARGET).abs
	edcltool -f run_nmc_code.edcl -i eth1

clean:
	-$(SILENT_CLEAN)rm -f *.asmx; rm -f *.o; rm -f $(TARGET).abs $(TARGET).dump *.dep \
	*.ac *.map *~ *.abs *.lib
