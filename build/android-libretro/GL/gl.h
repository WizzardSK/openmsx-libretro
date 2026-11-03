/* The libretro core's Android build: the NDK has no desktop OpenGL headers,
 * GLEW's declare all of GL (build/platform-android.mk). */
#ifndef GLEW_NO_GLU
#define GLEW_NO_GLU
#endif
#include <GL/glew.h>
