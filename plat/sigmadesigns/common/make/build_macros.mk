# HOST_MAKE_C builds a C source file and generates the dependency file
#   $(1) = output directory
#   $(2) = source file (%.c)
define HOST_MAKE_C

$(eval OBJ := $(1)/$(patsubst %.c,%.o,$(notdir $(2))))

# We use order-only prerequisite to ensure that directory are created,
# but do not cause re-build every time a file is written.
$(OBJ): $(2) | $(dir $(OBJ))
	@echo "  HOSTCC  $$<"
	$$(Q)gcc $$(HOST_CFLAGS) $$(CFLAGS_$(notdir $(OBJ))) -g -Wall -c $$< -o $$@
endef

# HOST_MAKE_OBJS builds C source files
#   $(1) = output directory
#   $(2) = list of source files
define HOST_MAKE_OBJS
        $(eval C_OBJS := $(filter %.c,$(2)))
        $(eval $(foreach src,$(C_OBJS),$(call HOST_MAKE_C,$(1),$(src))))
endef

# HOST_MAKE_PREREQ defines the prerequisite targets and options to build host tools
# Arguments:
#   $(1) = sources
#   $(2) = build path
define HOST_MAKE_PREREQ
	$(eval BUILD_DIR  := $(2))
	$(eval SOURCES    := $(1))
        $(eval OBJS       := $(addprefix $(BUILD_DIR)/,$(call SOURCES_TO_OBJS,$(SOURCES))))

        # We use sort only to get a list of unique object directory names.
        # ordering is not relevant but sort removes duplicates.
        $(eval TEMP_OBJ_DIRS := $(sort $(BUILD_DIR)/ $(dir ${OBJS})))
        # The $(dir ) function leaves a trailing / on the directory names
        # We append a . then strip /. from each, to remove the trailing / characters
        # This gives names suitable for use as make rule targets.
        $(eval OBJ_DIRS   := $(subst /.,,$(addsuffix .,$(TEMP_OBJ_DIRS))))

	# Create generators for object directory structure
	$(eval $(foreach objd,${OBJ_DIRS},$(call MAKE_PREREQ_DIR,${objd})))

	# Targets to build objects
	$(eval $(call HOST_MAKE_OBJS,$(BUILD_DIR),$(SOURCES)))

endef

# HOST_MAKE_TOOL defines the targets and options to build host tool
# Arguments:
#   $(1) = tool name
#   $(2) = tool build path
define HOST_MAKE_TOOL
	$(eval BUILD_DIR  := $(2))
	$(eval SOURCES	  := $($(call uppercase,$(1))_SOURCES))
	$(eval PREREQS	  := $($(call uppercase,$(1))_PREREQS))
	$(eval OBJS       := $(addprefix $(BUILD_DIR)/,$(call SOURCES_TO_OBJS,$(SOURCES))))
	$(eval ELF	  := $(call addprefix,${BUILD_DIR}/,$(1)))
	$(eval TARGET	  := $(1))

$(OBJS) : $(PREREQS)

$(ELF) : $(OBJS)
	@echo "  GEN     $$@"
	$$(Q)gcc $$(HOST_LDFLAGS) $$(LDFLAGS_$(notdir $(ELF))) $(OBJS) -o $$@

.PHONY : $(TARGET)
$(TARGET) : $(ELF)

endef

###
# filechk is used to check if the content of a generated file is updated.
# Sample usage:
# define filechk_sample
#	echo $KERNELRELEASE
# endef
# version.h : Makefile
#	$(call filechk,sample)
# The rule defined shall write to stdout the content of the new file.
# The existing file will be compared with the new one.
# - If no file exist it is created
# - If the content differ the new file is used
# - If they are equal no change, and no timestamp update
# - stdin is piped in from the first prerequisite ($<) so one has
#   to specify a valid file as first prerequisite (often the kbuild file)
define filechk
	$(Q)set -e;				\
	echo '  CHK     $@';			\
	mkdir -p $(dir $@);			\
	$(filechk_$(1)) < $< > $@.tmp;		\
	if [ -r $@ ] && cmp -s $@ $@.tmp; then	\
		rm -f $@.tmp;			\
	else					\
		echo '  UPD     $@';		\
		mv -f $@.tmp $@;		\
	fi
endef

# BIN_2_H tool, convert bin file to c header file.
# Use filechk to avoid rebuilds when a header changes, but the resulting file
# doest not. Arguments:
#   $(1) = bin2h
#   $(2) = bin file (mandatory)
#   $(3) = name (mandatory)
#   $(4) = array size (optional)
#   $(5) = attributes (optional)
#   $(6) = cmds for extra content (optional)
define filechk_bin2h
	(set -e;	\
	 echo "/*";	\
	 echo " * Automatically generated file. DO NOT EDIT!";		\
	 echo " */";	\
	 echo "";	\
	 echo "#ifndef __$(call uppercase,$3)_H__";	\
	 echo "#define __$(call uppercase,$3)_H__";	\
	 echo "";	\
	 echo "#if defined __cplusplus || defined __cplusplus__";	\
	 echo "extern \"C\" {";	\
	 echo "#endif";	\
	 echo "";	\
	 echo "static const unsigned char $(3)[$(4)] $(5) = {";	\
	 od -v -An -tx1 -w8 $(2) | sed 's/\([[:alnum:]]\+\)/0x\1,/g';	\
	 echo "};"; \
	 echo "";	\
	 $(6)		\
	 echo "";	\
	 echo "#if defined __cplusplus || defined __cplusplus__";	\
	 echo "}";	\
	 echo "#endif";	\
	 echo "";	\
	 echo "#endif /*__$(call uppercase,$3)_H__*/";	\
	)
endef

#
# Get variable value from an make readable file.
# This is needed when the file containing that variable is created no earlier
# than this 'make', i.e. included by '-include'
# Arguments:
#   $(1)=make readable *.mk file
#   $(2)=variable name
#
define early_mk_val
`awk -F '=' '$$0 ~ /^$(2)/ {print $$2}' $(1)`
endef

#
# Return file length in decimal
# Arguments:
#   $(1)=file
define file_size
`wc -c < $(1)`
endef

#
# Check file size in Rule body (recipe)
# Exit make in case of oversize.
# Arguments:
#   $(1) = file
#   $(2) = max length
define recipe_chk_file_size
	$(Q)set -e;	\
	if [ "$(call file_size,$(1))" -gt "$(2)" ]; then	\
		echo "ERROR: '$(1)' has exceeded its limit ($(call file_size,$(1))/$(2))" \
		&& exit -1;	\
	fi
endef

