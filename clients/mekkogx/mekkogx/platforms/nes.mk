EXECUTABLE = $(R2R_PD)/$(PRODUCT_BASE).nes
LIBRARY = $(R2R_PD)/$(PRODUCT_BASE).$(PLATFORM).lib

MWD := $(realpath $(dir $(lastword $(MAKEFILE_LIST)))..)
include $(MWD)/common.mk
include $(MWD)/toolchains/cc65.mk

# An NES cart image (.nes) for the FujiNet NES cartridge: the project's
# linker config lays out the banks and the "FUJI" claim; a checker runs
# after the link (the project's executable-post).

r2r:: $(BUILD_EXEC) $(BUILD_LIB) $(R2R_EXTRA_DEPS_$(PLATFORM_UC))
	make -f $(PLATFORM_MK) $(PLATFORM)/r2r-post
