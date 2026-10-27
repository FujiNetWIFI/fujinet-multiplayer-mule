EXECUTABLE = $(R2R_PD)/$(PRODUCT_BASE).bin
LIBRARY = $(R2R_PD)/$(PRODUCT_BASE).$(PLATFORM).lib

MWD := $(realpath $(dir $(lastword $(MAKEFILE_LIST)))..)
include $(MWD)/common.mk
include $(MWD)/toolchains/cc65.mk

# An Atari 5200 cart image for the FujiNet 5200 cartridge: the project's
# linker config lays out the Super Cart's banks and the "FUJI" claim; the
# project's executable-post fills the banks' shared part, checks the image
# and writes the .a52 (the cart and MAME both take it).

r2r:: $(BUILD_EXEC) $(BUILD_LIB) $(R2R_EXTRA_DEPS_$(PLATFORM_UC))
	make -f $(PLATFORM_MK) $(PLATFORM)/r2r-post
