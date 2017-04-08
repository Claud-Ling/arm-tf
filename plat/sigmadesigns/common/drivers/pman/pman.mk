
include $(SD_PLAT_COM)/drivers/pman/pmansetting.mk

#
# PMAN related make flags
#
# PMAN Debug flag
DEBUG_PMAN		:=	0

# Proceed with make flags
$(eval $(call assert_boolean,DEBUG_PMAN))
ifeq (${DEBUG_DDR},1)
  $(eval $(call add_define,DEBUG_PMAN))
endif

SD_PMAN_SOURCES		:=	$(SD_PLAT_COM)/drivers/pman/pman_sec.c	\
				$(SD_PLAT_COM)/drivers/pman/pman_sec_tab.c

# Dependencies
$(SD_PLAT_COM)/drivers/pman/pman_sec_tab.c : $(pmansetting-header)

