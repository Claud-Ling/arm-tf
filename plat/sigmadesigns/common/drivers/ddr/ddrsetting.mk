
include ${SD_PLAT_COM}/drivers/ddr/tools/tools.mk
-include $(autoconf-mk)

#
# DDR setting related make flags
#
# ddr_table location
DDR_TABLE		:=	${SD_PLAT_COM}/drivers/ddr/ddr_table
# DDR setting force configuring flag
CONFIG_DDR		:=	0

# Proceed with make flags
$(eval $(call assert_boolean,CONFIG_DDR))
ifeq (${CONFIG_DDR},1)
  FORCED_DDR_CONFIG	:=	FORCE
else
  FORCED_DDR_CONFIG	:=
endif

ddrconfig-script	:=	${SD_PLAT_COM}/drivers/ddr/scripts/ddr_configure.sh
ddresolve-script	:=	${SD_PLAT_COM}/drivers/ddr/scripts/resolve_setting.sh
ddrsetting-file		:=	${SD_BUILD_GEN}/tuning.dat
ddrsetting-bin		:=	${SD_BUILD_GEN}/ddrsetting.bin
ddrsetting-header	:=	${SD_BUILD_GEN}/ddrsetting.h
ddrtable-conf		:=	.ddr_table_selection
ddrtable-root		:=	${DDR_TABLE}

#
# Arguments:
#   $(1) = path to ddr_table
#   $(2) = ddrtable configure file
#   $(3) = ddrsetting file
#   $(4) = forced flag (config-ddr, optional)
#
define config_ddrsetting
	$(Q)set -e;	\
	if [ ${CONFIG_DDR} -eq 1 ]; then	\
		$(ddrconfig-script) $(1) $(2) $(3) config-ddr;	\
	else	\
		$(ddrconfig-script) $(1) $(2) $(3);	\
	fi
endef

#
# Arguments:
#   $(1) = ddrsetting file name
#   $(2) = header file with CONFIG_RT_xx
#
define resolve_name
	${ddresolve-script} $(1);
endef

$(ddrsetting-file) : $(ddrconfig-script) $(FORCED_DDR_CONFIG)
	$(Q)mkdir -p $(dir $@)
	$(call config_ddrsetting,$(ddrtable-root),$(ddrtable-conf),$(ddrsetting-file))

$(ddrsetting-bin) : $(ddrsetting-file) ${SD_BUILD_TOOL}/${GENREGTABLE}
	$(Q)set -e;	\
	mkdir -p $(dir $@);	\
	${SD_BUILD_TOOL}/${GENREGTABLE}  -g ${SD_PLAT_COM}/drivers/ddr/tools/config.ini	\
					-p 0 -c 1000 -m 800 $< $@;

$(ddrsetting-header) : $(ddrsetting-bin) $(resolve-script) $(autoconf-mk)
	$(Q)mkdir -p $(dir $@)
	$(call recipe_chk_file_size,$<,$(call early_mk_val,$(autoconf-mk),SD_RT_SIZE))
	$(call filechk,bin2h,$<,reg_tables,$(call early_mk_val,$(autoconf-mk),SD_RT_SIZE),"__section\(\"reg_tab\"\) __aligned\(16\) __used",$(call resolve_name,$(shell basename $(shell cat $(ddrtable-conf)))))

