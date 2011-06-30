//========================================================================
// GLFW - An OpenGL library
// Platform:    Win32/WGL
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


//////////////////////////////////////////////////////////////////////////
//////                       GLFW internal API                      //////
//////////////////////////////////////////////////////////////////////////

//========================================================================
// Return GL_TRUE if joystick is present, otherwise GL_FALSE
//========================================================================

static GLboolean isJoystickPresent(int joy)
{
    JOYINFO ji;

    // Is it a valid stick ID (Windows don't support more than 16 sticks)?
    if (joy < GLFW_JOYSTICK_1 || joy > GLFW_JOYSTICK_16)
        return GL_FALSE;

    // Is the joystick present?
    if (_glfw_joyGetPos(joy - GLFW_JOYSTICK_1, &ji) != JOYERR_NOERROR)
        return GL_FALSE;

    return GL_TRUE;
}


//========================================================================
// Calculate normalized joystick position
//========================================================================

static float calcJoystickPos(DWORD pos, DWORD min, DWORD max)
{
    float fpos = (float) pos;
    float fmin = (float) min;
    float fmax = (float) max;

    return (2.f * (fpos - fmin) / (fmax - fmin)) - 1.f;
}


//////////////////////////////////////////////////////////////////////////
//////                       GLFW platform API                      //////
//////////////////////////////////////////////////////////////////////////

//========================================================================
// Determine joystick capabilities
//========================================================================

int _glfwPlatformGetJoystickParam(int joy, int param)
{
    JOYCAPS jc;

    if (!isJoystickPresent(joy))
        return 0;

    // We got this far, the joystick is present
    if (param == GLFW_PRESENT)
        return GL_TRUE;

    // Get joystick capabilities
    _glfw_joyGetDevCaps(joy - GLFW_JOYSTICK_1, &jc, sizeof(JOYCAPS));

    switch (param)
    {
        case GLFW_AXES:
            // Return number of joystick axes
            return jc.wNumAxes;

        case GLFW_BUTTONS:
            // Return number of joystick axes
            return jc.wNumButtons;

        default:
            break;
    }

    return 0;
}


//========================================================================
// Get joystick axis positions
//========================================================================

int _glfwPlatformGetJoystickPos(int joy, float* pos, int numaxes)
{
    JOYCAPS jc;
    JOYINFOEX ji;
    int axis;

    if (!isJoystickPresent(joy))
        return 0;

    // Get joystick capabilities
    _glfw_joyGetDevCaps(joy - GLFW_JOYSTICK_1, &jc, sizeof(JOYCAPS));

    // Get joystick state
    ji.dwSize = sizeof(JOYINFOEX);
    ji.dwFlags = JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ |
                 JOY_RETURNR | JOY_RETURNU | JOY_RETURNV;
    _glfw_joyGetPosEx(joy - GLFW_JOYSTICK_1, &ji);

    // Get position values for all axes
    axis = 0;
    if (axis < numaxes)
        pos[axis++] = calcJoystickPos(ji.dwXpos, jc.wXmin, jc.wXmax);

    if (axis < numaxes)
        pos[axis++] = -calcJoystickPos(ji.dwYpos, jc.wYmin, jc.wYmax);

    if (axis < numaxes && jc.wCaps & JOYCAPS_HASZ)
        pos[axis++] = calcJoystickPos(ji.dwZpos, jc.wZmin, jc.wZmax);

    if (axis < numaxes && jc.wCaps & JOYCAPS_HASR)
        pos[axis++] = calcJoystickPos(ji.dwRpos, jc.wRmin, jc.wRmax);

    if (axis < numaxes && jc.wCaps & JOYCAPS_HASU)
        pos[axis++] = calcJoystickPos(ji.dwUpos, jc.wUmin, jc.wUmax);

    if (axis < numaxes && jc.wCaps & JOYCAPS_HASV)
        pos[axis++] = -calcJoystickPos(ji.dwVpos, jc.wVmin, jc.wVmax);

    return axis;
}


//========================================================================
// Get joystick button states
//========================================================================

int _glfwPlatformGetJoystickButtons(int joy, unsigned char* buttons,
                                    int numbuttons)
{
    JOYCAPS jc;
    JOYINFOEX ji;
    int button;

    if (!isJoystickPresent(joy))
        return 0;

    // Get joystick capabilities
    _glfw_joyGetDevCaps(joy - GLFW_JOYSTICK_1, &jc, sizeof(JOYCAPS));

    // Get joystick state
    ji.dwSize = sizeof(JOYINFOEX);
    ji.dwFlags = JOY_RETURNBUTTONS;
    _glfw_joyGetPosEx(joy - GLFW_JOYSTICK_1, &ji);

    // Get states of all requested buttons
    for (button = 0;  button < numbuttons && button < (int) jc.wNumButtons;  button++)
    {
        buttons[button] = (unsigned char)
            (ji.dwButtons & (1UL << button) ? GLFW_PRESS : GLFW_RELEASE);
    }

    return button;
}

