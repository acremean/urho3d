//========================================================================
// GLFW - An OpenGL library
// Platform:    X11/GLX
// API version: 3.0
// WWW:         http://www.glfw.org/
//------------------------------------------------------------------------
// Copyright (c) 2002-2006 Marcus Geelnard
// Copyright (c) 2006-2010 Camilla Berglund <elmindreda@elmindreda.org>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
//========================================================================

#include "internal.h"


//========================================================================
// Initialise timer
//========================================================================

void _glfwInitTimer(void)
{
    struct timeval tv;

    // "Resolution" is 1 us
    _glfwLibrary.X11.timer.resolution = 1e-6;

    // Set start-time for timer
    gettimeofday(&tv, NULL);
    _glfwLibrary.X11.timer.t0 = (long long) tv.tv_sec * (long long) 1000000 +
		                        (long long) tv.tv_usec;
}


//////////////////////////////////////////////////////////////////////////
//////                       GLFW platform API                      //////
//////////////////////////////////////////////////////////////////////////

//========================================================================
// Return timer value in seconds
//========================================================================

double _glfwPlatformGetTime(void)
{
    long long t;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    t = (long long) tv.tv_sec * (long long) 1000000 +
	(long long) tv.tv_usec;

    return (double)(t - _glfwLibrary.X11.timer.t0) * _glfwLibrary.X11.timer.resolution;
}


//========================================================================
// Set timer value in seconds
//========================================================================

void _glfwPlatformSetTime(double t)
{
    long long t0;
    struct timeval  tv;

    gettimeofday(&tv, NULL);
    t0 = (long long) tv.tv_sec * (long long) 1000000 +
	 (long long) tv.tv_usec;

    // Calulate new starting time
    _glfwLibrary.X11.timer.t0 = t0 - (long long)(t / _glfwLibrary.X11.timer.resolution);
}

