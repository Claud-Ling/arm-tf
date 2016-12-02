
fusedatamap_file	:=	${SD_PLAT_SOC}/res/fuse_data_map.fd
fusedatamap_header	:=	${SD_BUILD_GEN}/fuse_data_map.h

#
# Arguments:
#   $(1)=fusedatamap
#   $(2)=source fd file
#
define filechk_fusedatamap
	(set -e;	\
	 perl -Iperl ${SD_PLAT_COM}/tools/perl/fusedatamap.pl $(2) ${SD_BUILD_GEN}	\
	)
endef

$(fusedatamap_header) : $(fusedatamap_file) $(BITFIELDS_DEPS_otp) ${SD_PLAT_COM}/tools/perl/fusedatamap.pl
	$(Q)mkdir -p $(dir $@)
	$(call filechk,fusedatamap,$<)

.PHONY : sd-fusedatamap
sd-fusedatamap : $(fusedatamap_header)

