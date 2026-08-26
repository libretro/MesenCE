LOCAL_PATH := $(call my-dir)

ROOT_DIR     := $(LOCAL_PATH)/../..
CORE_DIR     := $(ROOT_DIR)/Core
LIBRETRO_DIR := $(ROOT_DIR)/Libretro
LUA_DIR      := $(ROOT_DIR)/Lua
SEVENZIP_DIR := $(ROOT_DIR)/SevenZip
UTIL_DIR     := $(ROOT_DIR)/Utilities

# The source list is derived the same way Makefile.libretro does it, so the
# Android build never goes stale when files are added or removed.
SOURCES_CXX := $(LIBRETRO_DIR)/libretro.cpp \
               $(LIBRETRO_DIR)/VfsFile.cpp \
               $(shell find $(CORE_DIR) -name '*.cpp') \
               $(shell find $(UTIL_DIR) -name '*.cpp')

# unix.c/unixdgram.c/unixstream.c/serial.c are optional luasocket modules the
# core never registers (and they don't build everywhere) - skip them
SOURCES_C   := $(shell find $(SEVENZIP_DIR) -name '*.c') \
               $(shell find $(UTIL_DIR) -name '*.c') \
               $(filter-out %/unix.c %/unixdgram.c %/unixstream.c %/serial.c,$(shell find $(LUA_DIR) -name '*.c'))

COREFLAGS := -DLIBRETRO -I$(ROOT_DIR) -I$(CORE_DIR) -I$(UTIL_DIR)

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
  COREFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

include $(CLEAR_VARS)
LOCAL_MODULE       := retro
LOCAL_SRC_FILES    := $(SOURCES_CXX) $(SOURCES_C)
LOCAL_CFLAGS       := $(COREFLAGS)
LOCAL_CXXFLAGS     := $(COREFLAGS) -std=c++17
LOCAL_LDFLAGS      := -Wl,-version-script=$(LIBRETRO_DIR)/link.T
LOCAL_CPP_FEATURES := exceptions rtti
include $(BUILD_SHARED_LIBRARY)
