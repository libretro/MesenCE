APP_STL := c++_static
APP_ABI := all

# std::filesystem (used by FolderUtilities) requires API 21+
APP_PLATFORM := android-21

# Don't strip debug builds
ifeq ($(NDK_DEBUG),1)
    cmd-strip :=
endif
