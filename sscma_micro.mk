# directory declaration
LIB_SSCMA_MICRO_DIR = $(LIBRARIES_ROOT)/sscma_micro


LIB_SSCMA_MICRO_DIR += $(LIBRARIES_ROOT)/sscma_micro/core
LIB_SSCMA_MICRO_DIR += $(LIBRARIES_ROOT)/sscma_micro/core/data
LIB_SSCMA_MICRO_DIR += $(LIBRARIES_ROOT)/sscma_micro/core/utils
LIB_SSCMA_MICRO_DIR += $(LIBRARIES_ROOT)/sscma_micro/core/engine
LIB_SSCMA_MICRO_DIR += $(LIBRARIES_ROOT)/sscma_micro/core/algorithm
LIB_SSCMA_MICRO_DIR += $(LIBRARIES_ROOT)/sscma_micro/core/synchronize
LIB_SSCMA_MICRO_DIR += $(LIBRARIES_ROOT)/sscma_micro/porting
LIB_SSCMA_MICRO_DIR += $(LIBRARIES_ROOT)/sscma_micro/porting/we1
LIB_SSCMA_MICRO_DIR += $(LIBRARIES_ROOT)/sscma_micro/porting/we1/boards
LIB_SSCMA_MICRO_DIR += $(LIBRARIES_ROOT)/sscma_micro/porting/we1/drivers
LIB_SSCMA_MICRO_DIR += $(LIBRARIES_ROOT)/sscma_micro/sscma
LIB_SSCMA_MICRO_DIR += $(LIBRARIES_ROOT)/sscma_micro/sscma/callback
LIB_SSCMA_MICRO_DIR += $(LIBRARIES_ROOT)/sscma_micro/sscma/interpreter
LIB_SSCMA_MICRO_DIR += $(LIBRARIES_ROOT)/sscma_micro/sscma/repl
LIB_SSCMA_MICRO_DIR += $(LIBRARIES_ROOT)/sscma_micro/third_party/FlashDB


LIB_SSCMA_MICRO_ASMSRCDIR	= $(LIB_SSCMA_MICRO_DIR)
LIB_SSCMA_MICRO_CSRCDIR	= $(LIB_SSCMA_MICRO_DIR)
LIB_SSCMA_MICRO_CXXSRCSDIR = $(LIB_SSCMA_MICRO_DIR)
LIB_SSCMA_MICRO_INCDIR	= $(LIB_SSCMA_MICRO_DIR)

# find all the source files in the target directories
LIB_SSCMA_MICRO_CSRCS = $(call get_csrcs, $(LIB_SSCMA_MICRO_CSRCDIR))
LIB_SSCMA_MICRO_CXXSRCS = $(call get_cxxsrcs, $(LIB_SSCMA_MICRO_CXXSRCSDIR))
LIB_SSCMA_MICRO_ASMSRCS = $(call get_asmsrcs, $(LIB_SSCMA_MICRO_ASMSRCDIR))

# get object files
LIB_SSCMA_MICRO_COBJS = $(call get_relobjs, $(LIB_SSCMA_MICRO_CSRCS))
LIB_SSCMA_MICRO_CXXOBJS = $(call get_relobjs, $(LIB_SSCMA_MICRO_CXXSRCS))
LIB_SSCMA_MICRO_ASMOBJS = $(call get_relobjs, $(LIB_SSCMA_MICRO_ASMSRCS))
LIB_SSCMA_MICRO_OBJS = $(LIB_SSCMA_MICRO_COBJS) $(LIB_SSCMA_MICRO_ASMOBJS) $(LIB_SSCMA_MICRO_CXXOBJS)

# get dependency files
LIB_SSCMA_MICRO_DEPS = $(call get_deps, $(LIB_SSCMA_MICRO_OBJS))

# extra macros to be defined
LIB_SSCMA_MICRO_DEFINES = -DLIB_SSCMA_MICRO

# genearte library
ifeq ($(C_LIB_FORCE_PREBUILT), y)
override LIB_SSCMA_MICRO_OBJS:=
endif
SSCMA_MICRO_LIB_NAME = libsscma_micro.a
LIB_SSCMA_MICRO := $(subst /,$(PS), $(strip $(OUT_DIR)/$(SSCMA_MICRO_LIB_NAME)))

# library generation rule
$(LIB_SSCMA_MICRO): $(LIB_SSCMA_MICRO_OBJS)
	$(TRACE_ARCHIVE)
ifeq "$(strip $(LIB_SSCMA_MICRO_OBJS))" ""
	$(CP) $(PREBUILT_LIB)$(SSCMA_MICRO_LIB_NAME) $(LIB_SSCMA_MICRO)
else
	$(Q)$(AR) $(AR_OPT) $@ $(LIB_SSCMA_MICRO_OBJS)
	$(CP) $(LIB_SSCMA_MICRO) $(PREBUILT_LIB)$(SSCMA_MICRO_LIB_NAME)
endif

# specific compile rules
# user can add rules to compile this library
# if not rules specified to this library, it will use default compiling rules

# library Definitions
LIB_INCDIR += $(LIB_SSCMA_MICRO_INCDIR)
LIB_CSRCDIR += $(LIB_SSCMA_MICRO_CSRCDIR)
LIB_CXXSRCDIR += $(LIB_SSCMA_MICRO_CXXSRCDIR)
LIB_ASMSRCDIR += $(LIB_SSCMA_MICRO_ASMSRCDIR)

LIB_CSRCS += $(LIB_SSCMA_MICRO_CSRCS)
LIB_CXXSRCS += $(LIB_SSCMA_MICRO_CXXSRCS)
LIB_ASMSRCS += $(LIB_SSCMA_MICRO_ASMSRCS)
LIB_ALLSRCS += $(LIB_SSCMA_MICRO_CSRCS) $(LIB_SSCMA_MICRO_ASMSRCS)

LIB_COBJS += $(LIB_SSCMA_MICRO_COBJS)
LIB_CXXOBJS += $(LIB_SSCMA_MICRO_CXXOBJS)
LIB_ASMOBJS += $(LIB_SSCMA_MICRO_ASMOBJS)
LIB_ALLOBJS += $(LIB_SSCMA_MICRO_OBJS)

LIB_DEFINES += $(LIB_SSCMA_MICRO_DEFINES)
LIB_DEPS += $(LIB_SSCMA_MICRO_DEPS)
LIB_LIBS += $(LIB_SSCMA_MICRO)
