
include ${SD_PLAT_COM}/drivers/otp/fusemap.mk
include ${SD_PLAT_COM}/drivers/otp/fusehelpers.mk

SD_OTP_SOURCES	:=	${SD_PLAT_COM}/drivers/otp/effuse.c	\
			${SD_PLAT_COM}/drivers/otp/otp.c

${SD_PLAT_COM}/drivers/otp/otp.c : sd-fusedatamap sd-fusehelpers
