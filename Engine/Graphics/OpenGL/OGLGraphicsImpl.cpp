//
// Urho3D Engine
// Copyright (c) 2008-2011 Lasse ��rni
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "Precompiled.h"
#include "Graphics.h"
#include "GraphicsEvents.h"
#include "GraphicsImpl.h"

#include <GLee.h>

GraphicsImpl::GraphicsImpl() :
    instance_(GetModuleHandle(0)),
    window_(0),
    deviceContext_(0),
    renderContext_(0),
    activeTexture_(0),
    drawBuffers_(M_MAX_UNSIGNED),
    fbo_(0),
    enabledAttributes_(0),
    depthBits_(0),
    windowDepthBits_(0),
    fboBound_(false)
{
}

unsigned GraphicsImpl::GetDesktopBitsPerPixel()
{
    DEVMODE settings;
    EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &settings);
    return settings.dmBitsPerPel;
}

IntVector2 GraphicsImpl::GetDesktopResolution()
{
    DEVMODE settings;
    EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &settings);
    return IntVector2(settings.dmPelsWidth, settings.dmPelsHeight);
}