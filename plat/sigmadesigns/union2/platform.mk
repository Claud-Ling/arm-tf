#
# Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
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

# flags
COLD_BOOT_SINGLE_CPU		:=	0
PROGRAMMABLE_RESET_ADDRESS	:=	0
ENABLE_PLAT_COMPAT		:=	0

SD_PLAT			:=	plat/sigmadesigns
SD_PLAT_SOC		:=	${SD_PLAT}/${PLAT}
SD_PLAT_COM		:=	${SD_PLAT}/common

# bl32 default to prebuild tee.bin
ifeq ($(SPD),opteed)
BL32			:=	$(SD_PLAT_SOC)/prebuild/tee.bin
endif

# platform specific settings file (must be set before include sd_common.mk)
SD_PLAT_PMAN_FILE	:=	${SD_PLAT_SOC}/res/pman_table/pman_setting.txt
SD_PLAT_DDR_FILE	:=	${SD_PLAT_SOC}/res/ddr_table/PLL_ARM24m-DDR_2g-PANEL_lvds_none-OTH_veloce2_umac01_NI.txt

include ${SD_PLAT_COM}/sd_common.mk

BL1_SOURCES		+=	lib/cpus/aarch64/cortex_a53.S			\
				${SD_PLAT_SOC}/plat_pinshare.c			\
				${SD_PLAT_SOC}/plat_security.c

BL2_SOURCES		+=	${SD_PLAT_SOC}/plat_pinshare.c			\
				${SD_PLAT_SOC}/plat_security.c

BL31_SOURCES		+=	lib/cpus/aarch64/cortex_a53.S			\
				${SD_PLAT_SOC}/plat_psci_handlers.c		\
				${SD_PLAT_SOC}/plat_security.c			\
				${SD_PLAT_SOC}/plat_sip_calls.c

#Dependencies
${SD_PLAT_SOC}/plat_security.c : sd-dcsnsec sd-pmansec $(BITFIELDS_DEPS_cpu)

# Flag used by the platform port to determine the version of ARM GIC
# architecture to use for interrupt management in EL3.
ARM_GIC_ARCH		:=	3
$(eval $(call add_define,ARM_GIC_ARCH))

