
dcsnsec_file	:=	${SD_PLAT_SOC}/res/dcsn_sec.dcsd
dcsnsec_header	:=	${SD_BUILD_GEN}/dcsn_sec.h

#
# Arguments:
#   $(1)=dcsnsec
#   $(2)=source dcsd file
#
define filechk_dcsnsec
	(set -e;	\
	 perl -Iperl ${SD_PLAT_COM}/tools/perl/dcsnsec.pl $(2)	\
	)
endef

$(dcsnsec_header) : $(dcsnsec_file) ${SD_PLAT_COM}/tools/perl/dcsnsec.pl
	$(Q)mkdir -p $(dir $@)
	$(call filechk,dcsnsec,$<)

.PHONY : sd-dcsnsec
sd-dcsnsec : $(dcsnsec_header)

pmansec_file	:=	${SD_PLAT_SOC}/res/pman_security_no.pmsd
pmansec_header	:=	${SD_BUILD_GEN}/pman_security_no.h

#
# Arguments:
#   $(1)=dcsnsec
#   $(2)=source pmsd file
#
define filechk_pmansec
	(set -e;	\
	 perl -Iperl ${SD_PLAT_COM}/tools/perl/pmansec.pl $(2)	\
	)
endef

$(pmansec_header) : $(pmansec_file) ${SD_PLAT_COM}/tools/perl/pmansec.pl
	$(Q)mkdir -p $(dir $@)
	$(call filechk,pmansec,$<)

.PHONY : sd-pmansec
sd-pmansec : $(pmansec_header)

include ${SD_PLAT_COM}/drivers/pman/pman.mk

SD_SEC_SOURCES	:=	${SD_PLAT_COM}/drivers/dcsn/dcsn_sec.c	\
			${SD_PLAT_COM}/sd_security.c		\
			${SD_PMAN_SOURCES}

#Dependencies
${SD_PLAT_COM}/drivers/dcsn/dcsn_sec.c : sd-dcsnsec

${SD_PMAN_SOURCES} : sd-pmansec

#Flags
SECURE		:=	1
$(eval $(call assert_boolean,SECURE))
SD_SECURE	:=	$(SECURE)
$(eval $(call add_define,SD_SECURE))
