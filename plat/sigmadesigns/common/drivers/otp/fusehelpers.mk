
otp_rd_dir		:=	${SD_PLAT_SOC}/include/bitfields/otp
fusehelpers_header	:=	${SD_BUILD_GEN}/fuse_helpers.h
fieldhelpers_pl		:=	${SD_PLAT_COM}/tools/perl/fieldhelpers.pl

#
# Arguments:
#   $(1)=fusehelper_header
#   $(2)=otp_rd_dir
#
define filechk_fusehelpers
	(set -e;	\
	 perl -Iperl $(fieldhelpers_pl) $(2)	\
	)
endef

$(eval OTP_RD_FILES	:=	$(shell find $(otp_rd_dir) -name *.rd))

$(fusehelpers_header) : $(OTP_RD_FILES) $(fieldhelpers_pl)
	$(Q)mkdir -p $(dir $@)
	$(call filechk,fusehelpers,$(otp_rd_dir))

.PHONY : sd-fusehelpers
sd-fusehelpers : $(fusehelpers_header)
