
# MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MKFILE_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
TARGET_PLATFORM=$(shell uname)



SRC_ROOT=$(MKFILE_DIR)
OUTPUT_DIR=$(MKFILE_DIR)/bin
LIB_PATHS += -L$(MKFILE_DIR)/lib
HEAD_DIRS += $(MKFILE_DIR)/third/eice/src/

#functions
get_files =$(foreach dir,$(subst $(DIR_DIVIDER),$(MK_DIR_DIVIDER),$(1)),$(wildcard $(dir)$(MK_DIR_DIVIDER)$(2)))


# SRC_DIRS += $(MKFILE_DIR)/bridgeserver/ $(MKFILE_DIR)/xctools/
# C_SRC += $(call get_files,$(SRC_DIRS),*.c)
# CPP_SRC += $(call get_files,$(SRC_DIRS),*.cpp)
# ALL_C_SRC += $(C_SRC)
# ALL_CPP_SRC += $(CPP_SRC)
# HEAD_DIRS += $(SRC_DIRS)


COMMON_SRC_DIRS += $(MKFILE_DIR)/bridgeserver/ $(MKFILE_DIR)/xctools/
COMMON_C_SRC += $(call get_files,$(COMMON_SRC_DIRS),*.c)
COMMON_CPP_SRC += $(call get_files,$(COMMON_SRC_DIRS),*.cpp)
ALL_C_SRC += $(COMMON_C_SRC)
ALL_CPP_SRC += $(COMMON_CPP_SRC)
HEAD_DIRS += $(COMMON_SRC_DIRS)
COMMON_OBJ += $(foreach src, $(COMMON_CPP_SRC), $(addsuffix .o, $(basename $(src))))


# COMMON_CPP_SRC += src/xcutil.cpp src/xrtp_h264.cpp  src/xtrans_codec.cpp
# # COMMON_H += src/xcutil.h src/xrtp_defs.h src/xrtp_h264.h src/xtrans_codec.h
# COMMON_OBJ += $(foreach src, $(COMMON_CPP_SRC), $(addsuffix .o, $(basename $(src))))
# ALL_CPP_SRC += $(COMMON_CPP_SRC)


TEST_SRC_DIRS += $(MKFILE_DIR)/test/ 
TEST_CPP_SRC += $(call get_files,$(TEST_SRC_DIRS),*.cpp)
TEST_OBJS += $(foreach src, $(TEST_CPP_SRC), $(addsuffix .o, $(basename $(src))))
ALL_CPP_SRC += $(TEST_CPP_SRC)


HEAD_DIRS += /usr/local/include/
INCLUDE_DIRS += $(addprefix -I,$(HEAD_DIRS))



TARGET_TEST:=$(OUTPUT_DIR)/test.bin
TARGET_ccs:=$(OUTPUT_DIR)/ccs


TARGET_BRIDGE:=$(OUTPUT_DIR)/bridge
TARGETS += $(TARGET_BRIDGE)
BRIDGE_OBJ = $(MKFILE_DIR)/bridgeserver/main.o 
ALL_CC_OBJ += $(BRIDGE_OBJ)

TARGET_BRIDGE_TEST:=$(OUTPUT_DIR)/test.bin
TARGETS += $(TARGET_BRIDGE_TEST)
BRIDGE_TEST_OBJ = $(MKFILE_DIR)/test/test_main.o 
ALL_CC_OBJ += $(BRIDGE_TEST_OBJ)

TARGET_CCS:=$(OUTPUT_DIR)/ccs
TARGETS += $(TARGET_CCS)
CCS_OBJ = $(MKFILE_DIR)/test/ccs_main.o 
ALL_CC_OBJ += $(CCS_OBJ)




MY_FLAGS= -g -Wall
# for MacOS
MY_FLAGS += -fno-pie 


#CC=gcc
CC=g++

MV = mv
RM = rm -fr
STRIP = strip


ALL_C_OBJ += $(foreach src, $(ALL_C_SRC), $(addsuffix .o, $(basename $(src))))
ALL_CPP_OBJ += $(foreach cpp, $(ALL_CPP_SRC), $(addsuffix .o, $(basename $(cpp))))


# -fno-strict-aliasing remove the warning "dereferencing type-punned pointer will break strict-aliasing rules "
# 



#CFLAGS = -fPIC -fno-strict-aliasing  -Wall -O2 $(INCLUDE_DIRS) 
CFLAGS= $(INCLUDE_DIRS) $(MACROS)  $(MY_FLAGS) 
#CFLAGS += -fstack-protector 
#CFLAGS += -std=c++11
#CFLAGS += -std=c++0x


CPPFLAGS=$(INCLUDE_DIRS) $(MACROS)  $(MY_FLAGS) -std=c++11



LIB_PATHS+= -L/usr/local/lib 
MY_LD_FLAGS = $(LIB_PATHS)
MY_LD_FLAGS += -lstdc++ 
MY_LD_FLAGS += -lm
# MY_LD_FLAGS += -lboost_system
# MY_LD_FLAGS += -lhiredis
MY_LD_FLAGS += -lpthread
MY_LD_FLAGS += -lcrypto

MY_LD_FLAGS += -leice-full
MY_LD_FLAGS += -llog4cplus -lcppunit 

# MY_LD_FLAGS += -pthread
# MY_LD_FLAGS += -lrt 

MY_LD_FLAGS += -g -fno-pie


CFLAGS += $(shell "pkg-config libevent --cflags")
CPPFLAGS += $(shell pkg-config libevent --cflags)
MY_LD_FLAGS += $(shell pkg-config libevent --libs)

# CFLAGS += $(shell "pkg-config uuid --cflags")
# CPPFLAGS += $(shell pkg-config uuid --cflags)
# MY_LD_FLAGS += $(shell pkg-config uuid --libs)

# CFLAGS += $(shell "pkg-config speex --cflags")
# CPPFLAGS += $(shell pkg-config speex --cflags)
# MY_LD_FLAGS += $(shell pkg-config speex --libs)

# CFLAGS += $(shell "pkg-config opus --cflags")
# CPPFLAGS += $(shell pkg-config opus --cflags)
# MY_LD_FLAGS += $(shell pkg-config opus --libs)

ifneq '$(TARGET_PLATFORM)' 'Darwin'
#MY_FLAGS+= -std=c++11
LIB_PATHS += -L/usr/lib64 

CFLAGS += $(shell "pkg-config uuid --cflags")
CPPFLAGS += $(shell pkg-config uuid --cflags)
MY_LD_FLAGS += $(shell pkg-config uuid --libs)
else
#MY_FLAGS+= -std=c++0x
#MY_FLAGS+= -Wc++11-extensions

endif


all: $(TARGETS)
	@echo build success


$(TARGET_BRIDGE): $(BRIDGE_OBJ) $(COMMON_OBJ) $(COMMON_H) $(OUTPUT_DIR) 
	$(CC)  $(BRIDGE_OBJ) $(COMMON_OBJ) $(MY_LD_FLAGS) -o $@
	
$(TARGET_BRIDGE_TEST): $(BRIDGE_TEST_OBJ) $(TEST_OBJS) $(COMMON_OBJ) $(COMMON_H) $(OUTPUT_DIR) 
	$(CC)  $(BRIDGE_TEST_OBJ) $(TEST_OBJS) $(COMMON_OBJ) $(MY_LD_FLAGS) -o $@

$(TARGET_CCS): $(CCS_OBJ) $(TEST_OBJS) $(COMMON_OBJ) $(COMMON_H) $(OUTPUT_DIR) 
	$(CC)  $(CCS_OBJ) $(TEST_OBJS) $(COMMON_OBJ) $(MY_LD_FLAGS) -o $@


$(OUTPUT_DIR):
	mkdir -p $@



bd:$(C_OBJ)
	echo $<

$(ALL_C_OBJ):%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(ALL_CPP_OBJ):%.o:%.cpp
	$(CC) -c $(CPPFLAGS) $< -o $@

$(ALL_CC_OBJ):%.o:%.cc
	$(CC) -c $(CPPFLAGS) $< -o $@

	
.PHONY: clean print_info
clean:
	$(RM) $(TARGETS)
	$(RM) $(ALL_C_OBJ)
	$(RM) $(ALL_CPP_OBJ)
	$(RM) $(ALL_CC_OBJ)
#	$(RM) ./bin/mserver.dSYM

print:
	@echo TARGET_PLATFORM=$(TARGET_PLATFORM)
	@echo ----------------------
	@echo CC=$(CC)
	@echo ----------------------
	@echo CFLAGS=$(CFLAGS)
	@echo ----------------------
	@echo CPPFLAGS=$(CPPFLAGS)
	@echo ----------------------
	@echo C_SRC=$(C_SRC)
	@echo ----------------------
	@echo CPP_SRC=$(CPP_SRC)
	@echo ----------------------
	@echo C_OBJ=$(C_OBJ)
	@echo ----------------------
	@echo CPP_OBJ=$(CPP_OBJ)
	@echo ----------------------
	@echo ALL_C_SRC=[$(ALL_C_SRC)]
	@echo ----------------------
	@echo ALL_CPP_SRC=[$(ALL_CPP_SRC)]





