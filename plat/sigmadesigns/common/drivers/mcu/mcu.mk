
SD_MCU_GENERIC_SRCS		:=	${SD_PLAT_COM}/drivers/mcu/mcu_reg.c

SD_MCU_BOOT_SOURCES		:=	${SD_PLAT_COM}/drivers/mcu/mcu_boot.c	\
					${SD_MCU_GENERIC_SRCS}

SD_MCU_COMM_SOURCES		:=	${SD_PLAT_COM}/drivers/mcu/mcu_comm.c	\
					${SD_MCU_GENERIC_SRCS}

SD_MCU_SOURCES			:=	${SD_MCU_GENERIC_SRCS}			\
					$(filter-out ${SD_MCU_GENERIC_SRCS},${SD_MCU_BOOT_SOURCES}) \
					$(filter-out ${SD_MCU_GENERIC_SRCS},${SD_MCU_COMM_SOURCES})

${SD_MCU_SOURCES} : $(BITFIELDS_DEPS_mcu)

