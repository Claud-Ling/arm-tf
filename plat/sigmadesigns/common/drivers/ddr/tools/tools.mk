
include ${SD_PLAT_COM}/make/build_macros.mk

## Target sources list, given by variable named as 'uppercase(target)_SOURCES'
GENREGTABLE_SOURCES	:=	${SD_PLAT_COM}/drivers/ddr/tools/genregtable.c	\
				${SD_PLAT_COM}/drivers/ddr/tools/crc32.c	\
				${SD_PLAT_COM}/drivers/ddr/tools/getgroup.c

## Target name
GENREGTABLE		:= genregtable

SD_DDR_TOOL		:= $(GENREGTABLE)

$(eval $(foreach n,${SD_DDR_TOOL},$(call HOST_MAKE_TOOL,$(n),$(SD_BUILD_TOOL))))
