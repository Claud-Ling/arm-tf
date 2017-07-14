
turingreg_file		:=	${SD_PLAT_COM}/res/turing_reg.td
turingreg_header	:=	${SD_BUILD_GEN}/turing_reg.h

#
# Arguments:
#   $(1)=turingreg
#   $(2)=source fd file
#
define filechk_turingreg
	(set -e;	\
	 perl -Iperl ${SD_PLAT_COM}/tools/perl/turingreg.pl $(2) ${SD_BUILD_GEN}	\
	)
endef

$(turingreg_header) : $(turingreg_file) $(BITFIELDS_DEPS_turing) ${SD_PLAT_COM}/tools/perl/turingreg.pl
	$(Q)mkdir -p $(dir $@)
	$(call filechk,turingreg,$<)

.PHONY : sd-turingreg
sd-turingreg : $(turingreg_header)

