/*
Add GLEW.  From the website:
The OpenGL Extension Wrangler Library (GLEW) is a cross-platform open-source C/C++ extension loading library.
GLEW provides efficient run-time mechanisms for determining which OpenGL extensions are supported on the target platform.
OpenGL core and extension functionality is exposed in a single header file.
GLEW has been tested on a variety of operating systems, including Windows, Linux, Mac OS X, FreeBSD, Irix, and Solaris.
*/
#include "GL/glew.h"

// Add libraries
#include <string>
#include <vector>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <time.h>
#include <cstdio>
#include <map>
#include <list>
#define GLM_SWIZZLE	// Enable GLM Swizzling, must be before glm is included!
#include "glm\glm.hpp"  // Core GLM stuff, same as GLSL math.
#include "glm\ext.hpp"	// GLM extensions.

// Boost headers for setting up the data server
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

// Define pi for use in converting degrees to radians
#ifndef M_PI
#define M_PI 3.1415926535
#endif

// To switch between open and closed loop
extern int closed;

extern float BallOffsetNow;

void InitOpenGL(void);

void RenderFrame(int windowNum, int direction);

void GLShutdown(void);