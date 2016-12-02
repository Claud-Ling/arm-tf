
#
# Arguments:
#   $(1)=target header file
#   $(2)=source rd file
#
define check_bitfield
	$(Q)set -e;				\
	echo '  CHK     $(1)';			\
	perl -Iperl ${SD_PLAT_COM}/tools/perl/bitfield.pl $(2) > $(1).tmp;		\
	if [ -r $(1) ] && cmp -s $(1) $(1).tmp; then	\
		rm -f $(1).tmp;			\
	else					\
		echo '  UPD     $(1)';		\
		mv -f $(1).tmp $(1);		\
	fi
endef

#
# Parse one RD file and translate it to c header file.
# Arguments:
#   $(1)=rd file (input)
#   $(2)=src dir
#   $(3)=output dir
#
define MAKE_RD
	$(eval TARGET			:= $(subst $(2),$(3),$(patsubst %.rd,%.h,$(1))))
	$(eval BFGRP			:= $(firstword $(subst /, ,$(subst $(2),,$(1)))))
	$(eval BITFIELDS_DEPS		+= $(TARGET))
	$(eval BITFIELDS_DEPS_$(BFGRP)	+= $(TARGET))

$(TARGET) : $(1) ${SD_PLAT_COM}/tools/perl/bitfield.pl
	$(Q)mkdir -p $(dir $(TARGET))
	$(call check_bitfield,$$@,$$<)
endef

BITFIELDS_DEPS			:=

#
# Search RD files under sources dir, translate them to c headers
# and output them to output dir. Arguments:
#   $(1) source dir
#   $(2) output dir
define MAKE_BITFIELDS
	$(eval RD_FILES		:= $(shell find $(1) -name *.rd))

	$(eval $(foreach n,$(RD_FILES),$(call MAKE_RD,$(n),$(1),$(2))))
endef

$(eval $(call MAKE_BITFIELDS,${SD_PLAT_SOC}/include/bitfields,${SD_BUILD_GEN}))
$(eval $(call MAKE_BITFIELDS,${SD_PLAT_COM}/include/bitfields,${SD_BUILD_GEN}))

.PHONY : sd-bitfields
sd-bitfields : $(BITFIELDS_DEPS)

