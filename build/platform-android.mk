# Configuration for Android, for ARM.

# Does platform require symlinks? (it is used to link the openMSX executable
# from a location inside the $PATH, which means it is not applicable for
# Android platform))
USE_SYMLINK:=false

# For Android, a shared library must eventually be build
EXEEXT:=
LIBRARYEXT:=.so

ANDROID_TARGET_FLAGS:=-DANDROID -fPIC
TARGET_FLAGS+=$(ANDROID_TARGET_FLAGS)
CMAKE_COMPILE_FLAGS+=$(ANDROID_TARGET_FLAGS)

LINK_FLAGS+=-llog

# The libraries built with CMake (SDL3, SDL3_ttf) need the NDK's toolchain
# file, or CMake configures them for the build machine: the NDK is found from
# the compiler on the PATH (<ndk>/toolchains/llvm/prebuilt/<host>/bin/clang).
ANDROID_NDK_DIR:=$(abspath $(dir $(shell command -v $(firstword $(CC))))/../../../../..)
ANDROID_ABI_NAME:=$(if $(filter aarch64,$(OPENMSX_TARGET_CPU)),arm64-v8a,$(if $(filter arm,$(OPENMSX_TARGET_CPU)),armeabi-v7a,$(OPENMSX_TARGET_CPU)))
CMAKE_TARGET_FLAGS+=\
	-DCMAKE_TOOLCHAIN_FILE=$(ANDROID_NDK_DIR)/build/cmake/android.toolchain.cmake \
	-DANDROID_ABI=$(ANDROID_ABI_NAME) \
	-DANDROID_PLATFORM=24
#LDFLAGS+=--no-undefined

# Build a maximum set of components.
# See configure.py for LINK_MODE definition and usage
ifeq ($(LIBRETRO),true)
# The libretro core: openMSX's sources need GL (GLEW's headers); its
# Software output never calls it, so GLEW is built for EGL and the GL
# functions come from Android's GLES libraries (libraries.py). <GL/gl.h>
# is GLEW's header there (build/android-libretro).
LINK_MODE:=3RD_STA
TARGET_FLAGS+=-I$(PWD)/build/android-libretro -DGLEW_NO_GLU
# GLEW is not taken by the library probe there (no desktop GL to check its
# functions against): its static library and EGL/GLES named here
LINK_FLAGS+=$(PWD)/derived/$(OPENMSX_TARGET_CPU)-android-opt-3rd/3rdparty/install/lib/libGLEW.a -lGLESv3 -lEGL
else
LINK_MODE:=3RD_STA_GLES
endif

# Automatically select the cross compiler.
ifeq ($(origin CXX),default)
ifeq ($(OPENMSX_TARGET_CPU),arm)
CXX:=armv7a-linux-androideabi21-clang++
else
CXX:=$(OPENMSX_TARGET_CPU)-linux-android-clang++
endif
endif
