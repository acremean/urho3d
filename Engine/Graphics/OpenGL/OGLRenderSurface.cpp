//
// Copyright (c) 2008-2013 the Urho3D project.
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
#include "Camera.h"
#include "Graphics.h"
#include "GraphicsImpl.h"
#include "Log.h"
#include "RenderSurface.h"
#include "Scene.h"
#include "Texture.h"

#include "DebugNew.h"

#ifdef GL_ES_VERSION_2_0
#define GL_RENDERBUFFER_EXT GL_RENDERBUFFER
#define glGenRenderbuffersEXT glGenRenderbuffers
#define glBindRenderbufferEXT glBindRenderbuffer
#define glRenderbufferStorageEXT glRenderbufferStorage
#define glDeleteRenderbuffersEXT glDeleteRenderbuffers
#endif

namespace Urho3D
{

RenderSurface::RenderSurface(Texture* parentTexture, unsigned target) :
    parentTexture_(parentTexture),
    target_(target),
    renderBuffer_(0)
{
}

RenderSurface::~RenderSurface()
{
    Release();
}

void RenderSurface::SetViewport(Viewport* viewport)
{
    if (viewport)
        viewport_ = viewport;
}

void RenderSurface::SetLinkedRenderTarget(RenderSurface* renderTarget)
{
    if (renderTarget != this)
        linkedRenderTarget_ = renderTarget;
}

void RenderSurface::SetLinkedDepthStencil(RenderSurface* depthStencil)
{
    if (depthStencil != this)
        linkedDepthStencil_ = depthStencil;
}

bool RenderSurface::CreateRenderBuffer(unsigned width, unsigned height, unsigned format)
{
    Graphics* graphics = parentTexture_->GetGraphics();
    if (!graphics)
        return false;
    
    Release();
    
    glGenRenderbuffersEXT(1, &renderBuffer_);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderBuffer_);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, format, width, height);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
    return true;
}

void RenderSurface::OnDeviceLost()
{
    Graphics* graphics = parentTexture_->GetGraphics();
    if (!graphics)
        return;
    
    for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
    {
        if (graphics->GetRenderTarget(i) == this)
            graphics->ResetRenderTarget(i);
    }
    
    if (graphics->GetDepthStencil() == this)
        graphics->ResetDepthStencil();
    
    // Clean up also from non-active FBOs
    graphics->CleanupRenderSurface(this);
    
    renderBuffer_ = 0;
}

void RenderSurface::Release()
{
    Graphics* graphics = parentTexture_->GetGraphics();
    if (!graphics)
        return;
    
    for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
    {
        if (graphics->GetRenderTarget(i) == this)
            graphics->ResetRenderTarget(i);
    }
    
    if (graphics->GetDepthStencil() == this)
        graphics->ResetDepthStencil();
    
    // Clean up also from non-active FBOs
    graphics->CleanupRenderSurface(this);
    
    if (renderBuffer_)
    {
        glDeleteRenderbuffersEXT(1, &renderBuffer_);
        renderBuffer_ = 0;
    }
}

int RenderSurface::GetWidth() const
{
    return parentTexture_->GetWidth();
}

int RenderSurface::GetHeight() const
{
    return parentTexture_->GetHeight();
}

TextureUsage RenderSurface::GetUsage() const
{
    return parentTexture_->GetUsage();
}

}
