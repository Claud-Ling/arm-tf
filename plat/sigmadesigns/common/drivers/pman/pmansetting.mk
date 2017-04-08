
-include $(autoconf-mk)

# PMAN flags
#
# PMAN Secure table location
PMAN_TABLE		?=	$(SD_PLAT_SOC)/res/pman_table
# PMAN setting force configuring flag
CONFIG_PMAN		?=	0

# Proceed with make flags
$(eval $(call assert_boolean,CONFIG_PMAN))
ifeq (${CONFIG_PMAN},1)
  FORCED_PMAN_CONFIG	:=	FORCE
else
  FORCED_PMAN_CONFIG	:=
endif

ifeq (${V},0)
  PST_VERBOSE		:=
else
  PST_VERBOSE		:= -V
endif

pmanconfig-script	:=	$(SD_PLAT_COM)/drivers/pman/scripts/pman_configure.sh
pmansetting-file	:=	$(SD_BUILD_GEN)/pmansec.dat
pmansetting-bin		:=	$(SD_BUILD_GEN)/pmansetting.bin
pmansetting-header	:=	$(SD_BUILD_GEN)/pmansetting.h
pmantable-conf		:=	$(SD_BUILD_GEN)/pman_table_selection
pmantable-root		:=	$(PMAN_TABLE)

#
# configure pman settings, in priority:
#   1. force pmanconfig-script
#   2. use SD_PLAT_PMAN_FILE
#   3. pmanconfig-script
# Arguments:
#   $(1) = path to pman_table
#   $(2) = pmantable configure file
#   $(3) = pmansetting file
#   $(4) = forced flag (config-pman, optional)
#
define config_pmansetting
	$(Q)set -e;	\
	if [ $(CONFIG_PMAN) -eq 1 ]; then	\
		$(pmanconfig-script) $(1) $(2) $(3) config-pman;	\
	elif [ -e $(SD_PLAT_PMAN_FILE) ]; then	\
			echo "/$(notdir $(SD_PLAT_PMAN_FILE))" > $(pmantable-conf);	\
			cp $(SD_PLAT_PMAN_FILE) $(pmansetting-file);	\
	else	\
			$(pmanconfig-script) $(1) $(2) $(3);	\
	fi
endef

$(pmansetting-file) : $(pmanconfig-script) $(SD_PLAT_PMAN_FILE) $(FORCED_PMAN_CONFIG)
	$(Q)mkdir -p $(dir $@)
	$(call config_pmansetting,$(pmantable-root),$(pmantable-conf),$(pmansetting-file))

$(pmansetting-bin) : $(SD_BUILD_TOOL)/$(GENPST) $(pmansetting-file)
	$(Q)set -e;	\
	mkdir -p $(dir $@);	\
	$^ -o $@ $(PST_VERBOSE);

$(pmansetting-header) : $(pmansetting-bin) $(autoconf-mk)
	$(Q)mkdir -p $(dir $@)
	$(call recipe_chk_file_size,$<,$(call early_mk_val,$(autoconf-mk),SD_PST_SIZE))
	$(call filechk,bin2h,$<,pman_table,$(call early_mk_val,$(autoconf-mk),SD_PST_SIZE),"__section\(\"pman_tab\"\) __aligned\($(call early_mk_val,$(autoconf-mk),SD_PST_ALIGN)\) __used",)

