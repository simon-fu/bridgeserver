
# MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MKFILE_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
TARGET_PLATFORM=$(shell uname)

PJ_DIR_ := ..
LIBDIR := $(MKFILE_DIR)/lib
# export BINDIR := ./bin



ifdef PJ_PATH	
	PJ_DIR_ := $(PJ_PATH)
endif

ifdef LIB_PATH	
	LIBDIR := $(LIB_PATH)
endif

include $(PJ_DIR_)/build.mak
include $(PJ_DIR_)/version.mak
include $(PJDIR)/build/common.mak



RULES_MAK := $(PJDIR)/build/rules.mak

export TARGET_LIB := libeice-$(TARGET_NAME)$(LIBEXT)

PJLIB_LIB:=libpj-$(TARGET_NAME)$(LIBEXT)
PJLIB_UTIL_LIB:=libpjlib-util-$(TARGET_NAME)$(LIBEXT)
PJNATH_LIB:=libpjnath-$(TARGET_NAME)$(LIBEXT)

PJLIB_LIB_PATH:=$(PJDIR)/pjlib/lib/$(PJLIB_LIB)
PJLIB_UTIL_LIB_PATH:=$(PJDIR)/pjlib-util/lib/$(PJLIB_UTIL_LIB)
PJNATH_LIB_PATH:=$(PJDIR)/pjnath/lib/$(PJNATH_LIB)



ifeq ($(EICE_SHARED_LIBRARIES),)
else
TARGET_SONAME = libeice.$(SHLIB_SUFFIX)
export EICE_SONAME := libeice.$(SHLIB_SUFFIX)
export EICE_SHLIB := $(TARGET_SONAME).$(PJ_VERSION_MAJOR)
endif

MERG_LIBS:=$(PJLIB_LIB_PATH) $(PJLIB_UTIL_LIB_PATH) $(PJNATH_LIB_PATH) $(LIBDIR)/$(TARGET_LIB)
ARSCRIPT:=_tmp.ar
ARSCRIPT_PATH:=$(LIBDIR)/$(ARSCRIPT)
FULL_LIB_PATH:=$(LIBDIR)/libeice-full.a



###############################################################################
# Gather all flags.
#
export _CFLAGS 	:= $(CC_CFLAGS) $(OS_CFLAGS) $(HOST_CFLAGS) $(M_CFLAGS) \
		   $(CFLAGS) $(CC_INC)../include
export _CXXFLAGS:= $(_CFLAGS) $(CC_CXXFLAGS) $(OS_CXXFLAGS) $(M_CXXFLAGS) \
		   $(HOST_CXXFLAGS) $(CXXFLAGS) -std=c++11
export _LDFLAGS := $(CC_LDFLAGS) $(OS_LDFLAGS) $(M_LDFLAGS) $(HOST_LDFLAGS) \
		   $(APP_LDFLAGS) $(LDFLAGS) 


###############################################################################
# Defines for building library
#

SRC_DIRS = $(MKFILE_DIR)/src/ 
HEAD_DIRS = $(SRC_DIRS)
# HEAD_DIRS += /usr/local/include

#functions
get_files =$(foreach dir,$(subst $(DIR_DIVIDER),$(MK_DIR_DIVIDER),$(1)),$(wildcard $(dir)$(MK_DIR_DIVIDER)$(2)))

C_SRC += $(call get_files,$(SRC_DIRS),*.c)
CPP_SRC += $(call get_files,$(SRC_DIRS),*.cpp)
INCLUDE_DIRS = $(addprefix -I,$(HEAD_DIRS)) 


C_OBJ = $(foreach src, $(C_SRC), $(addsuffix .o, $(basename $(src))))
CPP_OBJ = $(foreach cpp, $(CPP_SRC), $(addsuffix .o, $(basename $(cpp))))


ifeq '$(TARGET_PLATFORM)' 'Darwin'
LIB_TOOL_CMD :=libtool -static -o
else
LIB_TOOL_CMD :=$(MKFILE_DIR)/merge_static_lib.sh mergelinux $(ARSCRIPT_PATH) 
endif


###############################################################################
# Main entry
#
#
all: $(LIBDIR)/$(TARGET_LIB) $(FULL_LIB_PATH) $(TEST_BIN_PATH) $(CCS_BIN_PATH) $(BRIDGE_BIN_PATH)
	@echo "=> $(FULL_LIB_PATH)"

.PHONY:all 

$(C_OBJ):%.o:%.c
	$(CC) -c -Ddebug $(_CFLAGS) $(APP_CFLAGS) $(INCLUDE_DIRS)  $< -o $@

$(CPP_OBJ):%.o:%.cpp
	$(CXX) -c $(_CXXFLAGS) $(APP_CFLAGS) $(INCLUDE_DIRS) $< -o $@
$(LIBDIR)/$(TARGET_LIB): $(C_OBJ) $(CPP_OBJ)
	mkdir -p $(LIBDIR)
#	$(CC) $(C_OBJ) $(CPP_OBJ) $(_LDFLAGS) -o $@
	$(AR) rv $@ $(C_OBJ) $(CPP_OBJ) 
	@echo make success

$(FULL_LIB_PATH): $(MERG_LIBS)
# 	$(SILENT)echo "CREATE $@" > $(ARSCRIPT_PATH)
# 	$(SILENT)for a in $(MERG_LIBS); do (echo "ADDLIB $$a" >> $(ARSCRIPT_PATH)); done
# #	$(SILENT)echo "ADDMOD $(OBJECTS)" >> $(ARSCRIPT_PATH)
# 	$(SILENT)echo "SAVE" >> $(ARSCRIPT_PATH)
# 	$(SILENT)echo "END" >> $(ARSCRIPT_PATH)
# 	$(SILENT)$(AR) -M < $(ARSCRIPT_PATH)
# 	$(RM) $(ARSCRIPT_PATH)
	$(LIB_TOOL_CMD) $@ $^



.PHONY: clean print_info
clean:		
	$(RM) $(C_OBJ)
	$(RM) $(CPP_OBJ)
	$(RM) $(LIBDIR)/$(TARGET_LIB)
	$(RM) $(FULL_LIB_PATH)	


print:
	@echo TARGET_PLATFORM=$(TARGET_PLATFORM)
	@echo ----------------------
	@echo CC=$(CC)
	@echo ----------------------
	@echo CFLAGS=$(_CFLAGS)
	@echo ----------------------
	@echo CPPFLAGS=$(_CXXFLAGS)
	@echo ----------------------
	@echo C_SRC=$(C_SRC)
	@echo ----------------------
	@echo CPP_SRC=$(CPP_SRC)
	@echo ----------------------
	@echo C_OBJ=$(C_OBJ)
	@echo ----------------------
	@echo CPP_OBJ=$(CPP_OBJ)
	@echo ----------------------
	@echo MKFILE_DIR=$(MKFILE_DIR)
	@echo MKFILE_PATH=$(MKFILE_PATH)





