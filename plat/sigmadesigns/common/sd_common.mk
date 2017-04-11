#
# Copyright (c) 2015-2016, ARM Limited and Contributors. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# Neither the name of ARM nor the names of its contributors may be used
# to endorse or promote products derived from this software without specific
# prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

# Proceed Paths
SD_BUILD_TOOL		:=	${BUILD_PLAT}/tools
SD_BUILD_GEN		:=	${BUILD_PLAT}/generated

include ${SD_PLAT_COM}/make/build_macros.mk
include ${SD_PLAT_COM}/make/config.mk

# Proceed bitfields
include ${SD_PLAT_COM}/make/bitfields.mk

# Proceed tools
include ${SD_PLAT_COM}/tools/tools.mk

# Proceed top level build flags
# By default, SCP images are needed.(TODO: disable it for less boot pain for now)
SD_LOAD_SCP_IMAGES	:=	0
# By default, SCP boot in BL33 and is not needed here
SD_BOOT_SCP		:=	0

# Proceed debug flag
ifeq (${DEBUG}, 0)
  SD_WITH_PROD		:=	1
  $(eval $(call add_define_val,WITH_PROD,$(SD_WITH_PROD)))
endif

# Add __ATF__ flag for sigma
__ATF__	:= 1
$(eval $(call assert_boolean,__ATF__))
$(eval $(call add_define,__ATF__))

PLAT_INCLUDES		:=	-Iinclude/common/tbbr				\
				-I${SD_PLAT_COM}/include			\
				-I${SD_PLAT_SOC}/include			\
				-I${SD_PLAT_SOC}/include/${ARCH}		\
				-I${SD_BUILD_GEN}

PLAT_BL_COMMON_SOURCES	:=	lib/xlat_tables/xlat_tables_common.c		\
				lib/xlat_tables/aarch64/xlat_tables.c		\
				plat/common/aarch64/plat_common.c		\
				drivers/console/aarch64/console.S		\
				${SD_PLAT_COM}/drivers/uart/aarch64/uart.S	\
				${SD_PLAT_COM}/aarch64/sd_helpers.S		\
				${SD_PLAT_COM}/aarch64/sd_common.c

BL1_SOURCES		+=	drivers/delay_timer/delay_timer.c		\
				drivers/io/io_block.c				\
				drivers/io/io_fip.c				\
				drivers/io/io_storage.c				\
				plat/common/aarch64/platform_up_stack.S		\
				${SD_PLAT_COM}/sd_bl1_setup.c			\
				${SD_PLAT_COM}/sd_io_storage.c			\
				${SD_PLAT_COM}/drivers/timer/timer.c		\
				${SD_PLAT_COM}/sd_smp.c				\
				${SD_PLAT_COM}/sd_pinshare.c

BL2_SOURCES		+=	drivers/delay_timer/delay_timer.c		\
				drivers/io/io_block.c				\
				drivers/io/io_fip.c				\
				drivers/io/io_storage.c				\
				plat/common/aarch64/platform_up_stack.S		\
				${SD_PLAT_COM}/sd_bl2_setup.c			\
				${SD_PLAT_COM}/sd_io_storage.c			\
				${SD_PLAT_COM}/drivers/timer/timer.c		\
				${SD_PLAT_COM}/sd_pinshare.c

SD_GIC_SOURCES		:=	drivers/arm/gic/common/gic_common.c		\
				drivers/arm/gic/v3/gicv3_main.c			\
				drivers/arm/gic/v3/gicv3_helpers.c		\
				plat/common/plat_gicv3.c			\
				${SD_PLAT_COM}/sd_gicv3.c

BL31_SOURCES		+=	drivers/delay_timer/delay_timer.c		\
				plat/common/aarch64/platform_mp_stack.S		\
				plat/common/plat_psci_common.c			\
				${SD_PLAT_COM}/sd_bl31_setup.c			\
				${SD_PLAT_COM}/sd_pm.c				\
				${SD_PLAT_COM}/sd_topology.c			\
				${SD_PLAT_COM}/sd_smp.c				\
				${SD_GIC_SOURCES}

ifneq (${TRUSTED_BOARD_BOOT},0)

  # Include common auth sources
  AUTH_SOURCES		:=	${SD_PLAT_COM}/auth_mod.c			\
				${SD_PLAT_COM}/lib/crypto/rsa.c			\
				${SD_PLAT_COM}/lib/crypto/bignum.c		\
				${SD_PLAT_COM}/lib/crypto/sha2.c

  BL1_SOURCES		+=	${AUTH_SOURCES}

  BL2_SOURCES		+=	${AUTH_SOURCES}

endif

#
# Process Flash Storage
# Support FLASH type
#    emmc
#    nand
#    nor
#
SD_FLASH_MMC		:=	1
SD_FLASH_NAND		:=	2
SD_FLASH_NOR		:=	3

#The target build boot device and flash type
#Boot device, supported value: emmc, nand, nor
#Default boot from emmc
BOOTDEV			:=	emmc
#Main storage, supported value: emmc, nand, nor
#Default use emmc
STORAGE			:=	emmc

#Other options
DEBUG_EMMC		:=	0
DUMMY_EMMC		:=	0
SD_DEBUG_EMMC		:=	$(DEBUG_EMMC)
SD_DUMMY_EMMC		:=	$(DUMMY_EMMC)

ifeq (${BOOTDEV}, emmc)
  SD_BOOTDEV		:=	${SD_FLASH_MMC}
  # Override storage anyway
  SD_STORAGE		:=	${SD_FLASH_MMC}
else ifeq (${BOOTDEV}, nand)
  SD_BOOTDEV		:=	${SD_FLASH_NAND}
  # Override storage anyway
  SD_STORAGE		:=	${SD_FLASH_NAND}
else ifeq (${BOOTDEV}, nor)
  ifeq (${DEBUG}, 1)
    $(warning "BL1 is buggy in debug mode in case of NOR boot!")
  endif
  SD_BOOTDEV		:=	${SD_FLASH_NOR}
  ifeq (${STORAGE}, emmc)
    SD_STORAGE		:=	${SD_FLASH_MMC}
  else ifeq (${STORAGE}, nand)
    SD_STORAGE		:=	${SD_FLASH_NAND}
  else ifeq (${STORAGE}, nor)
    SD_STORAGE		:=	${SD_FLASH_NOR}
  else
    $(error "Unknown storage: ${STORAGE}, Usage: make STORAGE=<emmc|nand|nor> ...")
  endif
else
  $(error "Unknown boot device: ${BOOTDEV}, Usage: make BOOTDEV=<emmc|nand|nor> ...")
endif

SD_FLASH_SOURCES	:=	${SD_PLAT_COM}/sd_flash.c
ifeq (${SD_STORAGE}, ${SD_FLASH_MMC})
  $(eval $(call assert_boolean,DEBUG_EMMC))
  ifeq (${DEBUG_EMMC},1)
    $(eval $(call add_define,DEBUG_EMMC))
  endif
  $(eval $(call assert_boolean,DUMMY_EMMC))
  ifeq (${DUMMY_EMMC},1)
    SD_FLASH_SOURCES	+=	${SD_PLAT_COM}/drivers/emmc/dummy.c
  else
    SD_FLASH_SOURCES	+=	${SD_PLAT_COM}/drivers/emmc/emmc.c
  endif
else ifeq (${SD_STORAGE}, ${SD_FLASH_NAND})
  SD_FLASH_SOURCES	+=	${SD_PLAT_COM}/drivers/nand/nand.c
else ifeq (${SD_STORAGE}, ${SD_FLASH_NOR})
  SD_FLASH_SOURCES	+=	${SD_PLAT_COM}/drivers/nor/nor.c
else
    $(error "Unknown storage type: ${SD_STORAGE}!!")
endif

BL1_SOURCES		+=	${SD_FLASH_SOURCES}
BL2_SOURCES		+=	${SD_FLASH_SOURCES}

$(eval $(call add_define,SD_FLASH_MMC))
$(eval $(call add_define,SD_FLASH_NAND))
$(eval $(call add_define,SD_FLASH_NOR))
$(eval $(call add_define,SD_BOOTDEV))
$(eval $(call add_define,SD_STORAGE))

# Proceed ddr stuffs
include ${SD_PLAT_COM}/drivers/ddr/ddr.mk
BL1_SOURCES		+=	${SD_DDR_SOURCES}

# Proceed security stuffs
include ${SD_PLAT_COM}/security.mk
BL1_SOURCES		+=	${SD_SEC_SOURCES}
BL31_SOURCES		+=	${SD_SEC_SOURCES}

# Proceed otp stuffs
include ${SD_PLAT_COM}/drivers/otp/otp.mk
BL1_SOURCES		+=	${SD_OTP_SOURCES}
BL2_SOURCES		+=	${SD_OTP_SOURCES}
BL31_SOURCES		+=	${SD_OTP_SOURCES}

# Proceed SCP stuffs
include ${SD_PLAT_COM}/drivers/mcu/mcu.mk
BL31_SOURCES		+=	${SD_MCU_COMM_SOURCES}

$(eval $(call assert_boolean,SD_LOAD_SCP_IMAGES))
$(eval $(call add_define,SD_LOAD_SCP_IMAGES))

ifeq (${SD_LOAD_SCP_IMAGES},1)
  $(eval $(call FIP_ADD_IMG,SCP_BL2,--scp-fw))

  BL2_SOURCES		+=	${SD_MCU_BOOT_SOURCES}

  # Proceed SD_BOOT_SCP flag
  $(eval $(call assert_boolean,SD_BOOT_SCP))
  $(eval $(call add_define,SD_BOOT_SCP))

  ifeq (${SD_BOOT_SCP},1)
    BL2_SOURCES		+=	${SD_PLAT_COM}/sd_bdconf.c
  endif
endif

# Proceed with veloce stuffs
VELOCE			:=	0
$(eval $(call assert_boolean,VELOCE))
SD_VELOCE		:=	$(VELOCE)
$(eval $(call add_define,SD_VELOCE))

# Force Rule
.PHONY : FORCE
FORCE :
