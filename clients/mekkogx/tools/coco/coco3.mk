# coco3.mk -- mekkogx/platforms/coco.mk under another name: the CoCo 3
# pass of the CoCo sources (PLATFORM comes from this file's name, so it
# gets build/mule/coco3 and r2r/coco3 of its own).
EXEC_SUFFIX = .bin
DISK = $(R2R_PD)/$(PRODUCT_BASE).dsk
LIBRARY = $(R2R_PD)/lib$(PRODUCT_BASE).$(PLATFORM).a
DISK_TOOL = decb
DISK_TOOL_INFO = https://github.com/nitros9project/toolshed

MWD := $(CURDIR)/mekkogx
include $(MWD)/common.mk
include $(MWD)/toolchains/cmoc.mk

r2r:: $(BUILD_DISK) $(BUILD_LIB) $(R2R_EXTRA_DEPS)
	@make -f $(PLATFORM_MK) $(PLATFORM)/r2r-post

$(BUILD_DISK): $(DISK_EXECUTABLES) $(DISK_EXTRA_DEPS) $(DISK_EXTRA_FILES) | $(R2R_PD)
	$(RM) $@
	$(call require,$(DISK_TOOL),$(DISK_TOOL_INFO))
	$(DISK_TOOL) dskini $@
	@make -f $(PLATFORM_MK) $(PLATFORM)/disk-post
