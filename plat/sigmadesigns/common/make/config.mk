autoconf-mk	:=	${BUILD_PLAT}/generated/autoconf.mk

define filechk_autoconf
	(set -e;	\
	 echo "# Automatically generated file. DO NOT EDIT!";	\
	 $(CPP) $(CFLAGS) -DDO_DEPS_ONLY -dM $(2) | \
	 sed -n -f $(SD_PLAT_COM)/tools/define2mk.sed;	\
	)
endef

$(autoconf-mk)  : $(SD_PLAT_SOC)/include/sd_def.h
	$(Q)mkdir -p $(dir $@)
	$(call filechk,autoconf,$<)

testtest: $(SD_PLAT_SOC)/include/sd_def.h
	@echo "  GEN     $@"
	$(Q)mkdir -p $(dir $@)
	$(Q)set -e ; \
	: Extract the config macros ; \
	echo "# Automatically generated file. DO NOT EDIT!" > $@.tmp;	\
	$(CPP) $(CFLAGS) -DDO_DEPS_ONLY -dM $< | \
		sed -n -f $(SD_PLAT_COM)/tools/define2mk.sed >> $@.tmp;	\
	mv $@.tmp $@

