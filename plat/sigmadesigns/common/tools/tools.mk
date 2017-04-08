
include ${SD_PLAT_COM}/make/build_macros.mk
-include $(autoconf-mk)

SD_TOOL_SRC		:=	${SD_PLAT_COM}/tools/src

## Target sources list, given by variable named as 'uppercase(target)_SOURCES'
GENREGTABLE_SOURCES	:=	${SD_TOOL_SRC}/genregtable.c	\
				${SD_TOOL_SRC}/crc32.c	\
				${SD_TOOL_SRC}/getgroup.c

GENPST_SOURCES		:=	${SD_TOOL_SRC}/genpst.c	\
				${SD_TOOL_SRC}/crc32.c
## Target name
GENREGTABLE		:= genregtable
GENPST			:= genpst

## Target prerequisites
# filechk_genpst_conf is used to generate genst_conf.h.
# Arguments:
#   $(1) = genpst_conf
define filechk_genpst_conf
	(set -e;	\
	 echo "/*";	\
	 echo " * Automatically generated file. DO NOT EDIT!";		\
	 echo " */";	\
	 echo "";	\
	 echo "#ifndef __$(call uppercase,$1)_H__";	\
	 echo "#define __$(call uppercase,$1)_H__";	\
	 echo "";	\
	 echo "#define SD_PMAN_NR_GRPS ${SD_PMAN_NR_GRPS}";	\
	 echo "#define SD_PMAN_NR_RGNS ${SD_PMAN_NR_RGNS}";	\
	 echo "";	\
	 echo "#endif /*__$(call uppercase,$1)_H__*/";	\
	)
endef

GENPST_CONF_H		:=	${SD_BUILD_GEN}/genpst_conf.h
GENPST_PREREQS		:= 	${GENPST_CONF_H}

# the 1st prerequisite here is to cheat filechk helper only without no other meanings.
${GENPST_CONF_H} : $(lastword ${MAKEFILE_LIST}) FORCE
	$(call filechk,genpst_conf)

## Target FLAGS
# Don't mind NR_GRPS and NR_RGNS values here for they're only used to cheat make DEPS
# The right values shall be given in platform_def.h whatever
SD_PMAN_NR_GRPS		?=	1
SD_PMAN_NR_RGNS		?=	32
CFLAGS_genpst.o		:=	-I${SD_PLAT_COM}/include -include ${GENPST_CONF_H}

# tool targets list
SD_TOOLS		:= $(GENREGTABLE) $(GENPST)

# we use sort only to get a list of unique source names.
# ordering is not relevant but sort remove duplicates.
$(eval SD_TOOL_SOURCES	:= $(sort $(foreach n,${SD_TOOLS},$($(call uppercase,$(n))_SOURCES))))

# tools prerequisites
$(eval $(call HOST_MAKE_PREREQ,$(SD_TOOL_SOURCES),$(SD_BUILD_TOOL)))

# targets
$(eval $(foreach n,${SD_TOOLS},$(call HOST_MAKE_TOOL,$(n),$(SD_BUILD_TOOL))))
