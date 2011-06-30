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


void (*glXGetProcAddress(const GLubyte* procName))();
void (*glXGetProcAddressARB(const GLubyte* procName))();
void (*glXGetProcAddressEXT(const GLubyte* procName))();

// We support four different ways for getting addresses for GL/GLX
// extension functions: glXGetProcAddress, glXGetProcAddressARB,
// glXGetProcAddressEXT, and dlsym
#if   defined(_GLFW_HAS_GLXGETPROCADDRESSARB)
 #define _glfw_glXGetProcAddress(x) glXGetProcAddressARB(x)
#elif defined(_GLFW_HAS_GLXGETPROCADDRESS)
 #define _glfw_glXGetProcAddress(x) glXGetProcAddress(x)
#elif defined(_GLFW_HAS_GLXGETPROCADDRESSEXT)
 #define _glfw_glXGetProcAddress(x) glXGetProcAddressEXT(x)
#elif defined(_GLFW_HAS_DLOPEN)
 #define _glfw_glXGetProcAddress(x) dlsym(_glfwLibrary.X11.libGL, x)
 #define _GLFW_DLOPEN_LIBGL
#else
#define _glfw_glXGetProcAddress(x) NULL
#endif


//////////////////////////////////////////////////////////////////////////
//////                       GLFW internal API                      //////
//////////////////////////////////////////////////////////////////////////

//========================================================================
// Swap OpenGL buffers
//========================================================================

void _glfwPlatformSwapBuffers(void)
{
    glXSwapBuffers(_glfwLibrary.X11.display,
                   _glfwLibrary.currentWindow->X11.handle);
}


//========================================================================
// Set double buffering swap interval
//========================================================================

void _glfwPlatformSwapInterval(int interval)
{
    _GLFWwindow* window = _glfwLibrary.currentWindow;

    if (window->GLX.has_GLX_EXT_swap_control)
    {
        window->GLX.SwapIntervalEXT(_glfwLibrary.X11.display,
                                    window->X11.handle,
                                    interval);
    }
    else if (window->GLX.has_GLX_SGI_swap_control)
        window->GLX.SwapIntervalSGI(interval);
}


//========================================================================
// Check if an OpenGL extension is available at runtime
//========================================================================

int _glfwPlatformExtensionSupported(const char* extension)
{
    const GLubyte* extensions;

    // Get list of GLX extensions
    extensions = (const GLubyte*) glXQueryExtensionsString(_glfwLibrary.X11.display,
                                                           _glfwLibrary.X11.screen);
    if (extensions != NULL)
    {
        if (_glfwStringInExtensionString(extension, extensions))
            return GL_TRUE;
    }

    return GL_FALSE;
}


//========================================================================
// Get the function pointer to an OpenGL function
//========================================================================

void* _glfwPlatformGetProcAddress(const char* procname)
{
    return (void*) _glfw_glXGetProcAddress((const GLubyte*) procname);
}


//========================================================================
// Copies the specified OpenGL state categories from src to dst
//========================================================================

void _glfwPlatformCopyGLState(_GLFWwindow* src, _GLFWwindow* dst, unsigned long mask)
{
    glXCopyContext(_glfwLibrary.X11.display,
                   src->GLX.context,
                   dst->GLX.context,
                   mask);
}

