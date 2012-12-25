//
// Urho3D Engine
// Copyright (c) 2008-2012 Lasse Oorni
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
#include "AnimatedModel.h"
#include "Animation.h"
#include "AnimationController.h"
#include "BillboardSet.h"
#include "Camera.h"
#include "Context.h"
#include "CustomGeometry.h"
#include "DebugRenderer.h"
#include "DecalSet.h"
#include "Graphics.h"
#include "GraphicsEvents.h"
#include "GraphicsImpl.h"
#include "IndexBuffer.h"
#include "Log.h"
#include "Material.h"
#include "Mutex.h"
#include "Octree.h"
#include "ParticleEmitter.h"
#include "ProcessUtils.h"
#include "Profiler.h"
#include "RenderSurface.h"
#include "Shader.h"
#include "ShaderProgram.h"
#include "ShaderVariation.h"
#include "Skybox.h"
#include "StringUtils.h"
#include "Technique.h"
#include "Terrain.h"
#include "TerrainPatch.h"
#include "Texture2D.h"
#include "TextureCube.h"
#include "VertexBuffer.h"
#include "Zone.h"

#include <stdio.h>

#include "DebugNew.h"

#ifdef GL_ES_VERSION_2_0
#define GL_DEPTH_COMPONENT24 GL_DEPTH_COMPONENT24_OES
#define GL_FRAMEBUFFER_EXT GL_FRAMEBUFFER
#define GL_RENDERBUFFER_EXT GL_RENDERBUFFER
#define GL_COLOR_ATTACHMENT0_EXT GL_COLOR_ATTACHMENT0
#define GL_DEPTH_ATTACHMENT_EXT GL_DEPTH_ATTACHMENT
#define GL_STENCIL_ATTACHMENT_EXT GL_STENCIL_ATTACHMENT
#define GL_FRAMEBUFFER_COMPLETE_EXT GL_FRAMEBUFFER_COMPLETE
#define glClearDepth glClearDepthf
#define glBindFramebufferEXT glBindFramebuffer
#define glFramebufferTexture2DEXT glFramebufferTexture2D
#define glFramebufferRenderbufferEXT glFramebufferRenderbuffer
#define glGenFramebuffersEXT glGenFramebuffers
#define glDeleteFramebuffersEXT glDeleteFramebuffers
#define glCheckFramebufferStatusEXT glCheckFramebufferStatus
#endif

namespace Urho3D
{

static const unsigned glCmpFunc[] =
{
    GL_ALWAYS,
    GL_EQUAL,
    GL_NOTEQUAL,
    GL_LESS,
    GL_LEQUAL,
    GL_GREATER,
    GL_GEQUAL
};

static const unsigned glSrcBlend[] =
{
    GL_ONE,
    GL_ONE,
    GL_DST_COLOR,
    GL_SRC_ALPHA,
    GL_SRC_ALPHA,
    GL_ONE,
    GL_ONE_MINUS_DST_ALPHA
};

static const unsigned glDestBlend[] =
{
    GL_ZERO,
    GL_ONE,
    GL_ZERO,
    GL_ONE_MINUS_SRC_ALPHA,
    GL_ONE,
    GL_ONE_MINUS_SRC_ALPHA,
    GL_DST_ALPHA
};

#ifndef GL_ES_VERSION_2_0
static const unsigned glFillMode[] =
{
    GL_FILL,
    GL_LINE,
    GL_POINT
};
#endif

static const unsigned glStencilOps[] =
{
    GL_KEEP,
    GL_ZERO,
    GL_REPLACE,
    GL_INCR_WRAP,
    GL_DECR_WRAP
};

// Remap vertex attributes on OpenGL so that all usually needed attributes including skinning fit to the first 8.
// This avoids a skinning bug on GLES2 devices which only support 8.
static const unsigned glVertexAttrIndex[] =
{
    0, 1, 2, 3, 4, 8, 9, 5, 6, 7, 10, 11, 12
};

static const unsigned MAX_FRAMEBUFFER_AGE = 2000;

static unsigned numInstances = 0;

OBJECTTYPESTATIC(Graphics);

bool CheckExtension(const String& name)
{
    String extensions((const char*)glGetString(GL_EXTENSIONS));
    return extensions.Find(name) != String::NPOS;
}

Graphics::Graphics(Context* context_) :
    Object(context_),
    impl_(new GraphicsImpl()),
    externalWindow_(0),
    width_(0),
    height_(0),
    multiSample_(1),
    fullscreen_(false),
    vsync_(false),
    tripleBuffer_(false),
    lightPrepassSupport_(false),
    deferredSupport_(false),
    anisotropySupport_(false),
    dxtTextureSupport_(false),
    etcTextureSupport_(false),
    pvrtcTextureSupport_(false),
    numPrimitives_(0),
    numBatches_(0),
    maxScratchBufferRequest_(0),
    shadowMapFormat_(GL_DEPTH_COMPONENT16),
    hiresShadowMapFormat_(GL_DEPTH_COMPONENT24),
    defaultTextureFilterMode_(FILTER_BILINEAR)
{
    SetTextureUnitMappings();
    ResetCachedState();
    
    // If first instance in this process, initialize SDL under static mutex. Note that Graphics subsystem will also be in charge
    // of shutting down SDL as a whole, so it should be the last SDL-using subsystem (Audio and Input also use SDL) alive
    {
        MutexLock lock(GetStaticMutex());
        
        if (!numInstances)
            SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE);
        ++numInstances;
    }
}

Graphics::~Graphics()
{
    Close();
    
    delete impl_;
    impl_ = 0;
    
    // If last instance in this process, shut down SDL under static mutex
    {
        MutexLock lock(GetStaticMutex());
        
        --numInstances;
        if (!numInstances)
            SDL_Quit();
    }
}

void Graphics::SetExternalWindow(void* window)
{
    if (!impl_->window_)
        externalWindow_ = window;
    else
        LOGERROR("Window already opened, can not set external window");
}

void Graphics::SetWindowTitle(const String& windowTitle)
{
    windowTitle_ = windowTitle;
    if (impl_->window_)
        SDL_SetWindowTitle(impl_->window_, windowTitle_.CString());
}

bool Graphics::SetMode(int width, int height, bool fullscreen, bool vsync, bool tripleBuffer, int multiSample)
{
    PROFILE(SetScreenMode);
    
    multiSample = Clamp(multiSample, 1, 16);
    
    if (IsInitialized() && width == width_ && height == height_ && fullscreen == fullscreen_ &&
        vsync == vsync_ && tripleBuffer == tripleBuffer_ && multiSample == multiSample_)
        return true;
    
    // If only vsync changes, do not destroy/recreate the context
    if (IsInitialized() && width == width_ && height == height_ && fullscreen == fullscreen_ &&
        tripleBuffer == tripleBuffer_ && multiSample == multiSample_ && vsync != vsync_)
    {
        SDL_GL_SetSwapInterval(vsync ? 1 : 0);
        vsync_ = vsync;
        return true;
    }
    
    // If zero dimensions in windowed mode, set default. If zero in fullscreen, use desktop mode
    if (!width || !height)
    {
        if (!fullscreen)
        {
            width = 800;
            height = 600;
        }
        else
        {
            SDL_DisplayMode mode;
            SDL_GetDesktopDisplayMode(0, &mode);
            width = mode.w;
            height = mode.h;
        }
    }
    
    // With an external window, only the size can change after initial setup, so do not recreate context
    if (!externalWindow_ || !impl_->context_)
    {
        // Close the existing window and OpenGL context, mark GPU objects as lost
        Release(false, true);
    
        {
            // SDL window parameters are static, so need to operate under static lock
            MutexLock lock(GetStaticMutex());
        
            #ifdef IOS
            SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
            #endif
            
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
            SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
            #ifndef GL_ES_VERSION_2_0
            SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
            #else
            SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
            #endif
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
            
            if (multiSample > 1)
            {
                SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
                SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, multiSample);
            }
            else
            {
                SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
                SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
            }
            
            unsigned flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
            int x = fullscreen ? 0 : SDL_WINDOWPOS_UNDEFINED;
            int y = fullscreen ? 0 : SDL_WINDOWPOS_UNDEFINED;
            if (fullscreen)
                flags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS;
            
            // On iOS window needs to be resizable to handle orientation changes properly
            #ifdef IOS
            flags |= SDL_WINDOW_RESIZABLE;
            #endif
            
            for (;;)
            {
                if (!externalWindow_)
                    impl_->window_ = SDL_CreateWindow(windowTitle_.CString(), x, y, width, height, flags);
                else
                {
                    if (!impl_->window_)
                        impl_->window_ = SDL_CreateWindowFrom(externalWindow_, SDL_WINDOW_OPENGL);
                    fullscreen = false;
                }
                
                if (impl_->window_)
                    break;
                else
                {
                    if (multiSample > 1)
                    {
                        // If failed with multisampling, retry first without
                        multiSample = 1;
                        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
                        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
                    }
                    else
                    {
                        LOGERROR("Could not open window");
                        return false;
                    }
                }
            }
            
            // Create/restore context and GPU objects and set initial renderstate
            Restore();
            
            if (!impl_->context_)
            {
                LOGERROR("Could not create OpenGL context");
                return false;
            }
            
            // If OpenGL extensions not yet initialized, initialize now
            #ifndef GL_ES_VERSION_2_0
            if (!GLeeInitialized())
                GLeeInit();
            
            if (!_GLEE_VERSION_2_0)
            {
                LOGERROR("OpenGL 2.0 is required");
                Release(true, true);
                return false;
            }
            
            if (!CheckExtension("EXT_framebuffer_object") || !CheckExtension("EXT_packed_depth_stencil"))
            {
                LOGERROR("EXT_framebuffer_object and EXT_packed_depth_stencil OpenGL extensions are required");
                Release(true, true);
                return false;
            }
        
            dxtTextureSupport_ = CheckExtension("EXT_texture_compression_s3tc");
            anisotropySupport_ = CheckExtension("EXT_texture_filter_anisotropic");
            #else
            dxtTextureSupport_ = CheckExtension("EXT_texture_compression_dxt1");
            etcTextureSupport_ = CheckExtension("OES_compressed_ETC1_RGB8_texture");
            pvrtcTextureSupport_ = CheckExtension("IMG_texture_compression_pvrtc");
            #endif
        }
    }
    
    // Set vsync
    SDL_GL_SetSwapInterval(vsync ? 1 : 0);
    
    // Store the system FBO on IOS now
    #ifdef IOS
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&impl_->systemFbo_);
    #endif
    
    fullscreen_ = fullscreen;
    vsync_ = vsync;
    tripleBuffer_ = tripleBuffer;
    multiSample_ = multiSample;
    
    SDL_GetWindowSize(impl_->window_, &width_, &height_);
    
    // Reset rendertargets and viewport for the new screen mode
    ResetRenderTargets();
    
    // Clear the initial window contents to black
    Clear(CLEAR_COLOR);
    SDL_GL_SwapWindow(impl_->window_);
    
    CheckFeatureSupport();
    
    if (multiSample > 1)
        LOGINFO("Set screen mode " + String(width_) + "x" + String(height_) + " " + (fullscreen_ ? "fullscreen" : "windowed") +
        " multisample " + String(multiSample));
    else
        LOGINFO("Set screen mode " + String(width_) + "x" + String(height_) + " " + (fullscreen_ ? "fullscreen" : "windowed"));

    using namespace ScreenMode;
    
    VariantMap eventData;
    eventData[P_WIDTH] = width_;
    eventData[P_HEIGHT] = height_;
    eventData[P_FULLSCREEN] = fullscreen_;
    SendEvent(E_SCREENMODE, eventData);
    
    return true;
}

bool Graphics::SetMode(int width, int height)
{
    return SetMode(width, height, fullscreen_, vsync_, tripleBuffer_, multiSample_);
}

bool Graphics::ToggleFullscreen()
{
    return SetMode(width_, height_, !fullscreen_, vsync_, tripleBuffer_, multiSample_);
}

void Graphics::Close()
{
    if (!IsInitialized())
        return;
    
    // Actually close the window
    Release(true, true);
}

bool Graphics::TakeScreenShot(Image& destImage)
{
    PROFILE(TakeScreenShot);
    
    ResetRenderTargets();
    
    destImage.SetSize(width_, height_, 3);
    glReadPixels(0, 0, width_, height_, GL_RGB, GL_UNSIGNED_BYTE, destImage.GetData());
    
    return true;
}

bool Graphics::BeginFrame()
{
    if (!IsInitialized() || IsDeviceLost())
        return false;
    
    // If using an external window, check it for size changes, and reset screen mode if necessary
    if (externalWindow_)
    {
        int width, height;
        
        SDL_GetWindowSize(impl_->window_, &width, &height);
        if (width != width_ || height != height_)
            SetMode(width, height);
    }

    // Set default rendertarget and depth buffer
    ResetRenderTargets();
    
    // Cleanup textures from previous frame
    for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
        SetTexture(i, 0);
    
    // Enable color and depth write
    SetColorWrite(true);
    SetDepthWrite(true);
    
    numPrimitives_ = 0;
    numBatches_ = 0;
    
    SendEvent(E_BEGINRENDERING);
    
    return true;
}

void Graphics::EndFrame()
{
    if (!IsInitialized())
        return;
    
    PROFILE(Present);
    
    SendEvent(E_ENDRENDERING);
    
    SDL_GL_SwapWindow(impl_->window_);
    
    // Clean up FBO's that have not been used for a long time, and too large scratch buffers
    CleanupFramebuffers(false);
    CleanupScratchBuffers();
}

void Graphics::Clear(unsigned flags, const Color& color, float depth, unsigned stencil)
{
    if (impl_->fboDirty_)
        CommitFramebuffer();
    
    #ifdef GL_ES_VERSION_2_0
    flags &= ~CLEAR_STENCIL;
    #endif
    
    bool oldColorWrite = colorWrite_;
    bool oldDepthWrite = depthWrite_;
    
    if (flags & CLEAR_COLOR && !oldColorWrite)
        SetColorWrite(true);
    if (flags & CLEAR_DEPTH && !oldDepthWrite)
        SetDepthWrite(true);
    if (flags & CLEAR_STENCIL && stencilWriteMask_ != M_MAX_UNSIGNED)
        glStencilMask(M_MAX_UNSIGNED);
    
    unsigned glFlags = 0;
    if (flags & CLEAR_COLOR)
    {
        glFlags |= GL_COLOR_BUFFER_BIT;
        glClearColor(color.r_, color.g_, color.b_, color.a_);
    }
    if (flags & CLEAR_DEPTH)
    {
        glFlags |= GL_DEPTH_BUFFER_BIT;
        glClearDepth(depth);
    }
    if (flags & CLEAR_STENCIL)
    {
        glFlags |= GL_STENCIL_BUFFER_BIT;
        glClearStencil(stencil);
    }
    
    // If viewport is less than full screen, set a scissor to limit the clear
    /// \todo Any user-set scissor test will be lost
    IntVector2 viewSize = GetRenderTargetDimensions();
    if (viewport_.left_ != 0 || viewport_.top_ != 0 || viewport_.right_ != viewSize.x_ || viewport_.bottom_ != viewSize.y_)
        SetScissorTest(true, IntRect(0, 0, viewport_.Width(), viewport_.Height()));
    else
        SetScissorTest(false);
    
    glClear(glFlags);
    
    SetScissorTest(false);
    SetColorWrite(oldColorWrite);
    SetDepthWrite(oldDepthWrite);
    if (flags & CLEAR_STENCIL && stencilWriteMask_ != M_MAX_UNSIGNED)
        glStencilMask(stencilWriteMask_);
}

bool Graphics::ResolveToTexture(Texture2D* destination, const IntRect& viewport)
{
    if (!destination || !destination->GetRenderSurface() || destination->GetWidth() != width_ ||
        destination->GetHeight() != height_)
        return false;
    
    IntRect vpCopy = viewport;
    if (vpCopy.right_ <= vpCopy.left_)
        vpCopy.right_ = vpCopy.left_ + 1;
    if (vpCopy.bottom_ <= vpCopy.top_)
        vpCopy.bottom_ = vpCopy.top_ + 1;
    vpCopy.left_ = Clamp(vpCopy.left_, 0, width_);
    vpCopy.top_ = Clamp(vpCopy.top_, 0, height_);
    vpCopy.right_ = Clamp(vpCopy.right_, 0, width_);
    vpCopy.bottom_ = Clamp(vpCopy.bottom_, 0, height_);
    
    // Make sure the FBO is not in use
    ResetRenderTargets();
    
    // Use Direct3D convention with the vertical coordinates ie. 0 is top
    SetTextureForUpdate(destination);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, vpCopy.left_, height_ - vpCopy.bottom_, vpCopy.left_, height_ - vpCopy.bottom_,
        vpCopy.Width(), vpCopy.Height());
    SetTexture(0, 0);
    
    return true;
}

void Graphics::Draw(PrimitiveType type, unsigned vertexStart, unsigned vertexCount)
{
    if (!vertexCount)
        return;
    
    if (impl_->fboDirty_)
        CommitFramebuffer();
    
    unsigned primitiveCount = 0;
    
    switch (type)
    {
    case TRIANGLE_LIST:
        primitiveCount = vertexCount / 3;
        glDrawArrays(GL_TRIANGLES, vertexStart, vertexCount);
        break;
        
    case LINE_LIST:
        primitiveCount = vertexCount / 2;
        glDrawArrays(GL_LINES, vertexStart, vertexCount);
        break;
    }
    
    numPrimitives_ += primitiveCount;
    ++numBatches_;
}

void Graphics::Draw(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned minVertex, unsigned vertexCount)
{
    if (!indexCount || !indexBuffer_ || !indexBuffer_->GetGPUObject())
        return;
    
    if (impl_->fboDirty_)
        CommitFramebuffer();
    
    unsigned primitiveCount = 0;
    unsigned indexSize = indexBuffer_->GetIndexSize();
    
    switch (type)
    {
    case TRIANGLE_LIST:
        primitiveCount = indexCount / 3;
        if (indexSize == sizeof(unsigned short))
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, (const GLvoid*)(indexStart * indexSize));
        else
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (const GLvoid*)(indexStart * indexSize));
        break;
        
    case LINE_LIST:
        primitiveCount = indexCount / 2;
        if (indexSize == sizeof(unsigned short))
            glDrawElements(GL_LINES, indexCount, GL_UNSIGNED_SHORT, (const GLvoid*)(indexStart * indexSize));
        else
            glDrawElements(GL_LINES, indexCount, GL_UNSIGNED_INT, (const GLvoid*)(indexStart * indexSize));
        break;
    }
    
    numPrimitives_ += primitiveCount;
    ++numBatches_;
}

void Graphics::DrawInstanced(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned minVertex, unsigned vertexCount, unsigned instanceCount)
{
}

void Graphics::SetVertexBuffer(VertexBuffer* buffer)
{
    Vector<VertexBuffer*> vertexBuffers(1);
    PODVector<unsigned> elementMasks(1);
    vertexBuffers[0] = buffer;
    elementMasks[0] = MASK_DEFAULT;
    SetVertexBuffers(vertexBuffers, elementMasks);
}

bool Graphics::SetVertexBuffers(const Vector<VertexBuffer*>& buffers, const PODVector<unsigned>& elementMasks,
    unsigned instanceOffset)
{
    if (buffers.Size() > MAX_VERTEX_STREAMS)
    {
        LOGERROR("Too many vertex buffers");
        return false;
    }
    if (buffers.Size() != elementMasks.Size())
    {
        LOGERROR("Amount of element masks and vertex buffers does not match");
        return false;
    }
    
    bool changed = false;
    unsigned newAttributes = 0;
    
    for (unsigned i = 0; i < MAX_VERTEX_STREAMS; ++i)
    {
        VertexBuffer* buffer = 0;
        unsigned elementMask = 0;
        
        if (i < buffers.Size() && buffers[i])
        {
            buffer = buffers[i];
            if (elementMasks[i] == MASK_DEFAULT)
                elementMask = buffer->GetElementMask();
            else
                elementMask = buffer->GetElementMask() & elementMasks[i];
        }
        
        // If buffer and element mask have stayed the same, skip to the next buffer
        if (buffer == vertexBuffers_[i] && elementMask == elementMasks_[i] && !changed)
        {
            newAttributes |= elementMask;
            continue;
        }
        
        vertexBuffers_[i] = buffer;
        elementMasks_[i] = elementMask;
        changed = true;
        
        // Beware buffers with missing OpenGL objects, as binding a zero buffer object means accessing CPU memory for vertex data,
        // in which case the pointer will be invalid and cause a crash
        if (!buffer || !buffer->GetGPUObject())
            continue;
        
        glBindBuffer(GL_ARRAY_BUFFER, buffer->GetGPUObject());
        unsigned vertexSize = buffer->GetVertexSize();
        
        for (unsigned j = 0; j < MAX_VERTEX_ELEMENTS; ++j)
        {
            unsigned attrIndex = glVertexAttrIndex[j];
            unsigned elementBit = 1 << j;
            
            if (elementMask & elementBit)
            {
                newAttributes |= elementBit;
                
                // Enable attribute if not enabled yet
                if ((impl_->enabledAttributes_ & elementBit) == 0)
                {
                    glEnableVertexAttribArray(attrIndex);
                    impl_->enabledAttributes_ |= elementBit;
                }
                
                // Set the attribute pointer
                glVertexAttribPointer(attrIndex, VertexBuffer::elementComponents[j], VertexBuffer::elementType[j],
                    VertexBuffer::elementNormalize[j], vertexSize, (const GLvoid*)(buffer->GetElementOffset((VertexElement)j)));
            }
        }
    }
    
    if (!changed)
        return true;
    
    // Now check which vertex attributes should be disabled
    unsigned disableAttributes = impl_->enabledAttributes_ & (~newAttributes);
    unsigned disableIndex = 0;
    
    while (disableAttributes)
    {
        if (disableAttributes & 1)
        {
            glDisableVertexAttribArray(glVertexAttrIndex[disableIndex]);
            impl_->enabledAttributes_ &= ~(1 << disableIndex);
        }
        disableAttributes >>= 1;
        ++disableIndex;
    }
    
    return true;
}

bool Graphics::SetVertexBuffers(const Vector<SharedPtr<VertexBuffer> >& buffers, const PODVector<unsigned>&
    elementMasks, unsigned instanceOffset)
{
    if (buffers.Size() > MAX_VERTEX_STREAMS)
    {
        LOGERROR("Too many vertex buffers");
        return false;
    }
    if (buffers.Size() != elementMasks.Size())
    {
        LOGERROR("Amount of element masks and vertex buffers does not match");
        return false;
    }
    
    bool changed = false;
    unsigned newAttributes = 0;
    
    for (unsigned i = 0; i < MAX_VERTEX_STREAMS; ++i)
    {
        VertexBuffer* buffer = 0;
        unsigned elementMask = 0;
        
        if (i < buffers.Size() && buffers[i])
        {
            buffer = buffers[i];
            if (elementMasks[i] == MASK_DEFAULT)
                elementMask = buffer->GetElementMask();
            else
                elementMask = buffer->GetElementMask() & elementMasks[i];
        }
        
        // If buffer and element mask have stayed the same, skip to the next buffer
        if (buffer == vertexBuffers_[i] && elementMask == elementMasks_[i] && !changed)
        {
            newAttributes |= elementMask;
            continue;
        }
        
        vertexBuffers_[i] = buffer;
        elementMasks_[i] = elementMask;
        changed = true;
        
        // Beware buffers with missing OpenGL objects, as binding a zero buffer object means accessing CPU memory for vertex data,
        // in which case the pointer will be invalid and cause a crash
        if (!buffer || !buffer->GetGPUObject())
            continue;
        
        glBindBuffer(GL_ARRAY_BUFFER, buffer->GetGPUObject());
        unsigned vertexSize = buffer->GetVertexSize();
        
        for (unsigned j = 0; j < MAX_VERTEX_ELEMENTS; ++j)
        {
            unsigned attrIndex = glVertexAttrIndex[j];
            unsigned elementBit = 1 << j;
            
            if (elementMask & elementBit)
            {
                newAttributes |= elementBit;
                
                // Enable attribute if not enabled yet
                if ((impl_->enabledAttributes_ & elementBit) == 0)
                {
                    glEnableVertexAttribArray(attrIndex);
                    impl_->enabledAttributes_ |= elementBit;
                }
                
                // Set the attribute pointer
                glVertexAttribPointer(attrIndex, VertexBuffer::elementComponents[j], VertexBuffer::elementType[j],
                    VertexBuffer::elementNormalize[j], vertexSize, (const GLvoid*)(buffer->GetElementOffset((VertexElement)j)));
            }
        }
    }
    
    if (!changed)
        return true;
    
    // Now check which vertex attributes should be disabled
    unsigned disableAttributes = impl_->enabledAttributes_ & (~newAttributes);
    unsigned disableIndex = 0;
    
    while (disableAttributes)
    {
        if (disableAttributes & 1)
        {
            glDisableVertexAttribArray(glVertexAttrIndex[disableIndex]);
            impl_->enabledAttributes_ &= ~(1 << disableIndex);
        }
        disableAttributes >>= 1;
        ++disableIndex;
    }
    
    return true;
}

void Graphics::SetIndexBuffer(IndexBuffer* buffer)
{
    if (indexBuffer_ == buffer)
        return;
    
    if (buffer)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->GetGPUObject());
    else
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    indexBuffer_ = buffer;
}

void Graphics::SetShaders(ShaderVariation* vs, ShaderVariation* ps)
{
    if (vs == vertexShader_ && ps == pixelShader_)
        return;
    
    ClearParameterSources();
    
    // Compile the shaders now if not yet compiled. If already attempted, do not retry
    if (vs && !vs->IsCompiled())
    {
        if (vs->GetCompilerOutput().Empty())
        {
            PROFILE(CompileVertexShader);
            
            bool success = vs->Create();
            if (success)
                LOGDEBUG("Compiled vertex shader " + vs->GetName());
            else
            {
                LOGERROR("Failed to compile vertex shader " + vs->GetName() + ":\n" + vs->GetCompilerOutput());
                vs = 0;
            }
        }
        else
            vs = 0;
    }
    
    if (ps && !ps->IsCompiled())
    {
        if (ps->GetCompilerOutput().Empty())
        {
            PROFILE(CompilePixelShader);
            
            bool success = ps->Create();
            if (success)
                LOGDEBUG("Compiled pixel shader " + ps->GetName());
            else
            {
                LOGERROR("Failed to compile pixel shader " + ps->GetName() + ":\n" + ps->GetCompilerOutput());
                ps = 0;
            }
        }
        else
            ps = 0;
    }
    
    if (!vs || !ps)
    {
        glUseProgram(0);
        vertexShader_ = 0;
        pixelShader_ = 0;
        shaderProgram_ = 0;
    }
    else
    {
        vertexShader_ = vs;
        pixelShader_ = ps;
        
        Pair<ShaderVariation*, ShaderVariation*> combination(vs, ps);
        ShaderProgramMap::Iterator i = shaderPrograms_.Find(combination);
        
        if (i != shaderPrograms_.End())
        {
            // Use the existing linked program
            if (i->second_->IsLinked())
            {
                glUseProgram(i->second_->GetGPUObject());
                shaderProgram_ = i->second_;
            }
            else
            {
                glUseProgram(0);
                shaderProgram_ = 0;
            }
        }
        else
        {
            // Link a new combination
            SharedPtr<ShaderProgram> newProgram(new ShaderProgram(this, vs, ps));
            if (newProgram->Link())
            {
                LOGDEBUG("Linked vertex shader " + vs->GetName() + " and pixel shader " + ps->GetName());
                // Note: Link() calls glUseProgram() to set the texture sampler uniforms,
                // so it is not necessary to call it again
                shaderProgram_ = newProgram;
            }
            else
            {
                LOGERROR("Failed to link vertex shader " + vs->GetName() + " and pixel shader " + ps->GetName() + ":\n" +
                    newProgram->GetLinkerOutput());
                glUseProgram(0);
                shaderProgram_ = 0;
            }
            
            shaderPrograms_[combination] = newProgram;
        }
    }
}

void Graphics::SetShaderParameter(StringHash param, const float* data, unsigned count)
{
    if (shaderProgram_)
    {
        const ShaderParameter* info = shaderProgram_->GetParameter(param);
        if (info)
        {
            switch (info->type_)
            {
            case GL_FLOAT:
                glUniform1fv(info->location_, count, data);
                break;
                
            case GL_FLOAT_VEC2:
                glUniform2fv(info->location_, count / 2, data);
                break;
                
            case GL_FLOAT_VEC3:
                glUniform3fv(info->location_, count / 3, data);
                break;
                
            case GL_FLOAT_VEC4:
                glUniform4fv(info->location_, count / 4, data);
                break;
                
            case GL_FLOAT_MAT3:
                count = Min((int)count, (int)NUM_TEMP_MATRICES * 9);
                Matrix3::BulkTranspose(&tempMatrices3_[0].m00_, data, count / 9);
                glUniformMatrix3fv(info->location_, count / 9, GL_FALSE, tempMatrices3_[0].Data());
                break;
                
            case GL_FLOAT_MAT4:
                count = Min((int)count, (int)NUM_TEMP_MATRICES * 16);
                Matrix4::BulkTranspose(&tempMatrices4_[0].m00_, data, count / 16);
                glUniformMatrix4fv(info->location_, count / 16, GL_FALSE, tempMatrices4_[0].Data());
                break;
            }
        }
    }
}

void Graphics::SetShaderParameter(StringHash param, float value)
{
    if (shaderProgram_)
    {
        const ShaderParameter* info = shaderProgram_->GetParameter(param);
        if (info)
            glUniform1fv(info->location_, 1, &value);
    }
}

void Graphics::SetShaderParameter(StringHash param, const Color& color)
{
    SetShaderParameter(param, color.Data(), 4);
}

void Graphics::SetShaderParameter(StringHash param, const Matrix3& matrix)
{
    if (shaderProgram_)
    {
        const ShaderParameter* info = shaderProgram_->GetParameter(param);
        if (info)
            glUniformMatrix3fv(info->location_, 1, GL_FALSE, matrix.Transpose().Data());
    }
}

void Graphics::SetShaderParameter(StringHash param, const Vector3& vector)
{
    if (shaderProgram_)
    {
        const ShaderParameter* info = shaderProgram_->GetParameter(param);
        if (info)
        {
            // Check the uniform type to avoid mismatch
            switch (info->type_)
            {
            case GL_FLOAT:
                glUniform1fv(info->location_, 1, vector.Data());
                break;
                
            case GL_FLOAT_VEC2:
                glUniform2fv(info->location_, 1, vector.Data());
                break;
                
            case GL_FLOAT_VEC3:
                glUniform3fv(info->location_, 1, vector.Data());
                break;
            }
        }
    }
}

void Graphics::SetShaderParameter(StringHash param, const Matrix4& matrix)
{
    if (shaderProgram_)
    {
        const ShaderParameter* info = shaderProgram_->GetParameter(param);
        if (info)
            glUniformMatrix4fv(info->location_, 1, GL_FALSE, matrix.Transpose().Data());
    }
}

void Graphics::SetShaderParameter(StringHash param, const Vector4& vector)
{
    if (shaderProgram_)
    {
        const ShaderParameter* info = shaderProgram_->GetParameter(param);
        if (info)
        {
            // Check the uniform type to avoid mismatch
            switch (info->type_)
            {
            case GL_FLOAT:
                glUniform1fv(info->location_, 1, vector.Data());
                break;
                
            case GL_FLOAT_VEC2:
                glUniform2fv(info->location_, 1, vector.Data());
                break;
                
            case GL_FLOAT_VEC3:
                glUniform3fv(info->location_, 1, vector.Data());
                break;
                
            case GL_FLOAT_VEC4:
                glUniform4fv(info->location_, 1, vector.Data());
                break;
            }
        }
    }
}

void Graphics::SetShaderParameter(StringHash param, const Matrix3x4& matrix)
{
    if (shaderProgram_)
    {
        const ShaderParameter* info = shaderProgram_->GetParameter(param);
        if (info)
        {
            float data[16];
            data[0] = matrix.m00_;
            data[1] = matrix.m10_;
            data[2] = matrix.m20_;
            data[3] = 0.0f;
            data[4] = matrix.m01_;
            data[5] = matrix.m11_;
            data[6] = matrix.m21_;
            data[7] = 0.0f;
            data[8] = matrix.m02_;
            data[9] = matrix.m12_;
            data[10] = matrix.m22_;
            data[11] = 0.0f;
            data[12] = matrix.m03_;
            data[13] = matrix.m13_;
            data[14] = matrix.m23_;
            data[15] = 1.0f;
            
            glUniformMatrix4fv(info->location_, 1, GL_FALSE, data);
        }
    }
}

bool Graphics::NeedParameterUpdate(ShaderParameterGroup group, const void* source)
{
    if ((unsigned)shaderParameterSources_[group] == M_MAX_UNSIGNED || shaderParameterSources_[group] != source)
    {
        shaderParameterSources_[group] = source;
        return true;
    }
    else
        return false;
}

bool Graphics::HasShaderParameter(ShaderType type, StringHash param)
{
    return shaderProgram_ && shaderProgram_->HasParameter(param);
    
}

bool Graphics::HasTextureUnit(TextureUnit unit)
{
    return shaderProgram_ && shaderProgram_->HasTextureUnit(unit);
}


void Graphics::ClearParameterSource(ShaderParameterGroup group)
{
    shaderParameterSources_[group] = (const void*)M_MAX_UNSIGNED;
}

void Graphics::ClearParameterSources()
{
    for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
        shaderParameterSources_[i] = (const void*)M_MAX_UNSIGNED;
}

void Graphics::ClearTransformSources()
{
    shaderParameterSources_[SP_CAMERA] = (const void*)M_MAX_UNSIGNED;
    shaderParameterSources_[SP_OBJECTTRANSFORM] = (const void*)M_MAX_UNSIGNED;
}

void Graphics::CleanupShaderPrograms()
{
    for (ShaderProgramMap::Iterator i = shaderPrograms_.Begin(); i != shaderPrograms_.End();)
    {
        ShaderProgramMap::Iterator current = i++;
        ShaderVariation* vs = current->second_->GetVertexShader();
        ShaderVariation* ps = current->second_->GetPixelShader();
        
        if (!vs || !ps || !vs->GetGPUObject() || !ps->GetGPUObject())
            shaderPrograms_.Erase(current);
    }
}

void Graphics::SetTexture(unsigned index, Texture* texture)
{
    if (index >= MAX_TEXTURE_UNITS)
        return;
    
    // Check if texture is currently bound as a rendertarget. In that case, use its backup texture, or blank if not defined
    if (texture)
    {
        if (texture == viewTexture_ || (renderTargets_[0] && renderTargets_[0]->GetParentTexture() == texture))
            texture = texture->GetBackupTexture();
    }
    
    if (textures_[index] != texture)
    {
        if (impl_->activeTexture_ != index)
        {
            glActiveTexture(GL_TEXTURE0 + index);
            impl_->activeTexture_ = index;
        }
        
        if (texture)
        {
            unsigned glType = texture->GetTarget();
            if (glType != textureTypes_[index])
            {
                if (textureTypes_[index])
                    glDisable(textureTypes_[index]);
                
                glEnable(glType);
                textureTypes_[index] = glType;
            }
            
            glBindTexture(glType, texture->GetGPUObject());
            
            if (texture->GetParametersDirty())
                texture->UpdateParameters();
        }
        else
        {
            if (textureTypes_[index])
                glBindTexture(textureTypes_[index], 0);
        }
        
        textures_[index] = texture;
    }
    else
    {
        if (texture && texture->GetParametersDirty())
        {
            if (impl_->activeTexture_ != index)
            {
                glActiveTexture(GL_TEXTURE0 + index);
                impl_->activeTexture_ = index;
            }
            
            glBindTexture(texture->GetTarget(), texture->GetGPUObject());
            texture->UpdateParameters();
        }
    }
}

void Graphics::SetTextureForUpdate(Texture* texture)
{
    if (impl_->activeTexture_ != 0)
    {
        glActiveTexture(GL_TEXTURE0);
        impl_->activeTexture_ = 0;
    }
    
    glBindTexture(texture->GetTarget(), texture->GetGPUObject());
    textures_[0] = texture;
}

void Graphics::SetDefaultTextureFilterMode(TextureFilterMode mode)
{
    if (mode != defaultTextureFilterMode_)
    {
        defaultTextureFilterMode_ = mode;
        SetTextureParametersDirty();
    }
}

void Graphics::SetTextureAnisotropy(unsigned level)
{
    if (level != textureAnisotropy_)
    {
        textureAnisotropy_ = level;
        SetTextureParametersDirty();
    }
}

void Graphics::SetTextureParametersDirty()
{
    for (Vector<GPUObject*>::Iterator i = gpuObjects_.Begin(); i != gpuObjects_.End(); ++i)
    {
        Texture* texture = dynamic_cast<Texture*>(*i);
        if (texture)
            texture->SetParametersDirty();
    }
}

void Graphics::ResetRenderTargets()
{
    for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
        SetRenderTarget(i, (RenderSurface*)0);
    SetDepthStencil((RenderSurface*)0);
    SetViewport(IntRect(0, 0, width_, height_));
}

void Graphics::ResetRenderTarget(unsigned index)
{
    SetRenderTarget(index, (RenderSurface*)0);
}

void Graphics::ResetDepthStencil()
{
    SetDepthStencil((RenderSurface*)0);
}

void Graphics::SetRenderTarget(unsigned index, RenderSurface* renderTarget)
{
    if (index >= MAX_RENDERTARGETS)
        return;
    
    if (renderTarget != renderTargets_[index])
    {
        renderTargets_[index] = renderTarget;
        
        // If the rendertarget is also bound as a texture, replace with backup texture or null
        if (renderTarget)
        {
            Texture* parentTexture = renderTarget->GetParentTexture();
            
            for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
            {
                if (textures_[i] == parentTexture)
                    SetTexture(i, textures_[i]->GetBackupTexture());
            }
        }
        
        impl_->fboDirty_ = true;
    }
}

void Graphics::SetRenderTarget(unsigned index, Texture2D* texture)
{
    RenderSurface* renderTarget = 0;
    if (texture)
        renderTarget = texture->GetRenderSurface();
    
    SetRenderTarget(index, renderTarget);
}

void Graphics::SetDepthStencil(RenderSurface* depthStencil)
{
    // If we are using a rendertarget texture, it is required in OpenGL to also have an own depth-stencil
    // Create a new depth-stencil texture as necessary to be able to provide similar behaviour as Direct3D9
    if (renderTargets_[0] && !depthStencil)
    {
        int width = renderTargets_[0]->GetWidth();
        int height = renderTargets_[0]->GetHeight();
        
        // Direct3D9 default depth-stencil can not be used when rendertarget is larger than the window.
        // Check size similarly
        if (width <= width_ && height <= height_)
        {
            int searchKey = (width << 16) | height;
            HashMap<int, SharedPtr<Texture2D> >::Iterator i = depthTextures_.Find(searchKey);
            if (i != depthTextures_.End())
                depthStencil = i->second_->GetRenderSurface();
            else
            {
                SharedPtr<Texture2D> newDepthTexture(new Texture2D(context_));
                newDepthTexture->SetSize(width, height, GetDepthStencilFormat(), TEXTURE_DEPTHSTENCIL);
                depthTextures_[searchKey] = newDepthTexture;
                depthStencil = newDepthTexture->GetRenderSurface();
            }
        }
    }
    
    if (depthStencil != depthStencil_)
    {
        depthStencil_ = depthStencil;
        impl_->fboDirty_ = true;
    }
}

void Graphics::SetDepthStencil(Texture2D* texture)
{
    RenderSurface* depthStencil = 0;
    if (texture)
        depthStencil = texture->GetRenderSurface();
    
    SetDepthStencil(depthStencil);
}

void Graphics::SetViewTexture(Texture* texture)
{
    viewTexture_ = texture;
    
    if (viewTexture_)
    {
        for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
        {
            if (textures_[i] == viewTexture_)
                SetTexture(i, textures_[i]->GetBackupTexture());
        }
    }
}

void Graphics::SetViewport(const IntRect& rect)
{
    if (impl_->fboDirty_)
        CommitFramebuffer();
    
    IntVector2 rtSize = GetRenderTargetDimensions();
    
    IntRect rectCopy = rect;
    
    if (rectCopy.right_ <= rectCopy.left_)
        rectCopy.right_ = rectCopy.left_ + 1;
    if (rectCopy.bottom_ <= rectCopy.top_)
        rectCopy.bottom_ = rectCopy.top_ + 1;
    rectCopy.left_ = Clamp(rectCopy.left_, 0, rtSize.x_);
    rectCopy.top_ = Clamp(rectCopy.top_, 0, rtSize.y_);
    rectCopy.right_ = Clamp(rectCopy.right_, 0, rtSize.x_);
    rectCopy.bottom_ = Clamp(rectCopy.bottom_, 0, rtSize.y_);
    
    // Use Direct3D convention with the vertical coordinates ie. 0 is top
    glViewport(rectCopy.left_, rtSize.y_ - rectCopy.bottom_, rectCopy.Width(), rectCopy.Height());
    viewport_ = rectCopy;
    
    // Disable scissor test, needs to be re-enabled by the user
    SetScissorTest(false);
}

void Graphics::SetBlendMode(BlendMode mode)
{
    if (mode != blendMode_)
    {
        if (mode == BLEND_REPLACE)
            glDisable(GL_BLEND);
        else
        {
            glEnable(GL_BLEND);
            glBlendFunc(glSrcBlend[mode], glDestBlend[mode]);
        }
        
        blendMode_ = mode;
    }
}

void Graphics::SetColorWrite(bool enable)
{
    if (enable != colorWrite_)
    {
        if (enable)
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        else
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        
        colorWrite_ = enable;
    }
}

void Graphics::SetCullMode(CullMode mode)
{
    if (mode != cullMode_)
    {
        if (mode == CULL_NONE)
            glDisable(GL_CULL_FACE);
        else
        {
            // Use Direct3D convention, ie. clockwise vertices define a front face
            glEnable(GL_CULL_FACE);
            glCullFace(mode == CULL_CCW ? GL_FRONT : GL_BACK);
        }
        
        cullMode_ = mode;
    }
}

void Graphics::SetDepthBias(float constantBias, float slopeScaledBias)
{
    if (constantBias != constantDepthBias_ || slopeScaledBias != slopeScaledDepthBias_)
    {
        #ifndef GL_ES_VERSION_2_0
        if (slopeScaledBias != 0.0f)
        {
            // OpenGL constant bias is unreliable and dependant on depth buffer bitdepth, apply in the projection matrix instead
            float adjustedSlopeScaledBias = slopeScaledBias + 1.0f;
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(adjustedSlopeScaledBias, 0.0f);
        }
        else
            glDisable(GL_POLYGON_OFFSET_FILL);
        #endif
        
        constantDepthBias_ = constantBias;
        slopeScaledDepthBias_ = slopeScaledBias;
        shaderParameterSources_[SP_CAMERA] = (const void*)M_MAX_UNSIGNED;
    }
}

void Graphics::SetDepthTest(CompareMode mode)
{
    if (mode != depthTestMode_)
    {
        glDepthFunc(glCmpFunc[mode]);
        depthTestMode_ = mode;
    }
}

void Graphics::SetDepthWrite(bool enable)
{
    if (enable != depthWrite_)
    {
        glDepthMask(enable ? GL_TRUE : GL_FALSE);
        depthWrite_ = enable;
    }
}

void Graphics::SetFillMode(FillMode mode)
{
    #ifndef GL_ES_VERSION_2_0
    if (mode != fillMode_)
    {
        glPolygonMode(GL_FRONT_AND_BACK, glFillMode[mode]);
        fillMode_ = mode;
    }
    #endif
}

void Graphics::SetScissorTest(bool enable, const Rect& rect, bool borderInclusive)
{
    // During some light rendering loops, a full rect is toggled on/off repeatedly.
    // Disable scissor in that case to reduce state changes
    if (rect.min_.x_ <= 0.0f && rect.min_.y_ <= 0.0f && rect.max_.x_ >= 1.0f && rect.max_.y_ >= 1.0f)
        enable = false;
    
    if (enable)
    {
        IntVector2 rtSize(GetRenderTargetDimensions());
        IntVector2 viewSize(viewport_.Size());
        IntVector2 viewPos(viewport_.left_, viewport_.top_);
        IntRect intRect;
        int expand = borderInclusive ? 1 : 0;
        
        intRect.left_ = Clamp((int)((rect.min_.x_ + 1.0f) * 0.5f * viewSize.x_) + viewPos.x_, 0, rtSize.x_ - 1);
        intRect.top_ = Clamp((int)((-rect.max_.y_ + 1.0f) * 0.5f * viewSize.y_) + viewPos.y_, 0, rtSize.y_ - 1);
        intRect.right_ = Clamp((int)((rect.max_.x_ + 1.0f) * 0.5f * viewSize.x_) + viewPos.x_ + expand, 0, rtSize.x_);
        intRect.bottom_ = Clamp((int)((-rect.min_.y_ + 1.0f) * 0.5f * viewSize.y_) + viewPos.y_ + expand, 0, rtSize.y_);
        
        if (intRect.right_ == intRect.left_)
            intRect.right_++;
        if (intRect.bottom_ == intRect.top_)
            intRect.bottom_++;
        
        if (intRect.right_ < intRect.left_ || intRect.bottom_ < intRect.top_)
            enable = false;
        
        if (enable && scissorRect_ != intRect)
        {
            // Use Direct3D convention with the vertical coordinates ie. 0 is top
            glScissor(intRect.left_, rtSize.y_ - intRect.bottom_, intRect.Width(), intRect.Height());
            scissorRect_ = intRect;
        }
    }
    else
        scissorRect_ = IntRect::ZERO;
    
    if (enable != scissorTest_)
    {
        if (enable)
            glEnable(GL_SCISSOR_TEST);
        else
            glDisable(GL_SCISSOR_TEST);
        scissorTest_ = enable;
    }
}

void Graphics::SetScissorTest(bool enable, const IntRect& rect)
{
    IntVector2 rtSize(GetRenderTargetDimensions());
    IntVector2 viewSize(viewport_.Size());
    IntVector2 viewPos(viewport_.left_, viewport_.top_);
    
    if (enable)
    {
        IntRect intRect;
        intRect.left_ = Clamp(rect.left_ + viewPos.x_, 0, rtSize.x_ - 1);
        intRect.top_ = Clamp(rect.top_ + viewPos.y_, 0, rtSize.y_ - 1);
        intRect.right_ = Clamp(rect.right_ + viewPos.x_, 0, rtSize.x_);
        intRect.bottom_ = Clamp(rect.bottom_ + viewPos.y_, 0, rtSize.y_);
        
        if (intRect.right_ == intRect.left_)
            intRect.right_++;
        if (intRect.bottom_ == intRect.top_)
            intRect.bottom_++;
        
        if (intRect.right_ < intRect.left_ || intRect.bottom_ < intRect.top_)
            enable = false;
        
        if (enable && scissorRect_ != intRect)
        {
            // Use Direct3D convention with the vertical coordinates ie. 0 is top
            glScissor(intRect.left_, rtSize.y_ - intRect.bottom_, intRect.Width(), intRect.Height());
            scissorRect_ = intRect;
        }
    }
    else
        scissorRect_ = IntRect::ZERO;
    
    if (enable != scissorTest_)
    {
        if (enable)
            glEnable(GL_SCISSOR_TEST);
        else
            glDisable(GL_SCISSOR_TEST);
        scissorTest_ = enable;
    }
}

void Graphics::SetStreamFrequency(unsigned index, unsigned frequency)
{
}

void Graphics::ResetStreamFrequencies()
{
}

void Graphics::SetStencilTest(bool enable, CompareMode mode, StencilOp pass, StencilOp fail, StencilOp zFail, unsigned stencilRef, unsigned compareMask, unsigned writeMask)
{
    #ifndef GL_ES_VERSION_2_0
    if (enable != stencilTest_)
    {
        if (enable)
            glEnable(GL_STENCIL_TEST);
        else
            glDisable(GL_STENCIL_TEST);
        stencilTest_ = enable;
    }
    
    if (enable)
    {
        if (mode != stencilTestMode_ || stencilRef != stencilRef_ || compareMask != stencilCompareMask_)
        {
            glStencilFunc(glCmpFunc[mode], stencilRef, compareMask);
            stencilTestMode_ = mode;
            stencilRef_ = stencilRef;
            stencilCompareMask_ = compareMask;
        }
        if (writeMask != stencilWriteMask_)
        {
            glStencilMask(writeMask);
            stencilWriteMask_ = writeMask;
        }
        if (pass != stencilPass_ || fail != stencilFail_ || zFail != stencilZFail_)
        {
            glStencilOp(glStencilOps[fail], glStencilOps[zFail], glStencilOps[pass]);
            stencilPass_ = pass;
            stencilFail_ = fail;
            stencilZFail_ = zFail;
        }
    }
    #endif
}

void Graphics::SetForceSM2(bool enable)
{
}

bool Graphics::IsInitialized() const
{
    return impl_->window_ != 0;
}

bool Graphics::IsDeviceLost() const
{
    // On iOS treat window minimization as device loss, as it is forbidden to access OpenGL when minimized
    #ifdef IOS
    if (impl_->window_ && (SDL_GetWindowFlags(impl_->window_) & SDL_WINDOW_MINIMIZED) != 0)
        return true;
    #endif
    
    return impl_->context_ == 0;
}

PODVector<IntVector2> Graphics::GetResolutions() const
{
    PODVector<IntVector2> ret;
    unsigned numModes = SDL_GetNumDisplayModes(0);
    
    for (unsigned i = 0; i < numModes; ++i)
    {
        SDL_DisplayMode mode;
        SDL_GetDisplayMode(0, i, &mode);
        int width = mode.w;
        int height  = mode.h;
        
        // Store mode if unique
        bool unique = true;
        for (unsigned j = 0; j < ret.Size(); ++j)
        {
            if (ret[j].x_ == width && ret[j].y_ == height)
            {
                unique = false;
                break;
            }
        }
        
        if (unique)
            ret.Push(IntVector2(width, height));
    }
    
    return ret;
}

PODVector<int> Graphics::GetMultiSampleLevels() const
{
    PODVector<int> ret;
    // No multisampling always supported
    ret.Push(1);
    /// \todo Implement properly, if possible
    
    return ret;
}

unsigned Graphics::GetFormat(CompressedFormat format) const
{
    switch (format)
    {
    case CF_DXT1:
        return dxtTextureSupport_ ? GL_COMPRESSED_RGBA_S3TC_DXT1_EXT : 0;

    #ifndef GL_ES_VERSION_2_0
    case CF_DXT3:
        return dxtTextureSupport_ ? GL_COMPRESSED_RGBA_S3TC_DXT3_EXT : 0;
        
    case CF_DXT5:
        return dxtTextureSupport_ ? GL_COMPRESSED_RGBA_S3TC_DXT5_EXT : 0;
    #else
    case CF_ETC1:
        return etcTextureSupport_ ? GL_ETC1_RGB8_OES : 0;
        
    case CF_PVRTC_RGB_2BPP:
        return pvrtcTextureSupport_ ? COMPRESSED_RGB_PVRTC_2BPPV1_IMG : 0;
        
    case CF_PVRTC_RGB_4BPP:
        return pvrtcTextureSupport_ ? COMPRESSED_RGB_PVRTC_4BPPV1_IMG : 0;
        
    case CF_PVRTC_RGBA_2BPP:
        return pvrtcTextureSupport_ ? COMPRESSED_RGBA_PVRTC_2BPPV1_IMG : 0;
        
    case CF_PVRTC_RGBA_4BPP:
        return pvrtcTextureSupport_ ? COMPRESSED_RGBA_PVRTC_4BPPV1_IMG : 0;
    #endif
    }
    
    return 0;
}


VertexBuffer* Graphics::GetVertexBuffer(unsigned index) const
{
    return index < MAX_VERTEX_STREAMS ? vertexBuffers_[index] : 0;
}

TextureUnit Graphics::GetTextureUnit(const String& name)
{
    HashMap<String, TextureUnit>::Iterator i = textureUnits_.Find(name);
    if (i != textureUnits_.End())
        return i->second_;
    else
        return MAX_TEXTURE_UNITS;
}

const String& Graphics::GetTextureUnitName(TextureUnit unit)
{
    for (HashMap<String, TextureUnit>::Iterator i = textureUnits_.Begin(); i != textureUnits_.End(); ++i)
    {
        if (i->second_ == unit)
            return i->first_;
    }
    return String::EMPTY;
}

Texture* Graphics::GetTexture(unsigned index) const
{
    return index < MAX_TEXTURE_UNITS ? textures_[index] : 0;
}

RenderSurface* Graphics::GetRenderTarget(unsigned index) const
{
    return index < MAX_RENDERTARGETS ? renderTargets_[index] : 0;
}

IntVector2 Graphics::GetRenderTargetDimensions() const
{
    int width, height;
    
    if (renderTargets_[0])
    {
        width = renderTargets_[0]->GetWidth();
        height = renderTargets_[0]->GetHeight();
    }
    else if (depthStencil_)
    {
        width = depthStencil_->GetWidth();
        height = depthStencil_->GetHeight();
    }
    else
    {
        width = width_;
        height = height_;
    }
    
    return IntVector2(width, height);
}

void Graphics::AddGPUObject(GPUObject* object)
{
    gpuObjects_.Push(object);
}

void Graphics::RemoveGPUObject(GPUObject* object)
{
    gpuObjects_.Erase(gpuObjects_.Find(object));
}

void* Graphics::ReserveScratchBuffer(unsigned size)
{
    if (!size)
        return 0;
    
    if (size > maxScratchBufferRequest_)
        maxScratchBufferRequest_ = size;
    
    // First check for a free buffer that is large enough
    for (Vector<ScratchBuffer>::Iterator i = scratchBuffers_.Begin(); i != scratchBuffers_.End(); ++i)
    {
        if (!i->reserved_ && i->size_ >= size)
        {
            i->reserved_ = true;
            return i->data_.Get();
        }
    }
    
    // Then check if a free buffer can be resized
    for (Vector<ScratchBuffer>::Iterator i = scratchBuffers_.Begin(); i != scratchBuffers_.End(); ++i)
    {
        if (!i->reserved_)
        {
            i->data_ = new unsigned char[size];
            i->size_ = size;
            i->reserved_ = true;
            
            LOGDEBUG("Resized scratch buffer to size " + String(size));
            
            return i->data_.Get();
        }
    }
    
    // Finally allocate a new buffer
    ScratchBuffer newBuffer;
    newBuffer.data_ = new unsigned char[size];
    newBuffer.size_ = size;
    newBuffer.reserved_ = true;
    scratchBuffers_.Push(newBuffer);
    return newBuffer.data_.Get();
    
    LOGDEBUG("Allocated scratch buffer with size " + String(size));
}

void Graphics::FreeScratchBuffer(void* buffer)
{
    if (!buffer)
        return;
    
    for (Vector<ScratchBuffer>::Iterator i = scratchBuffers_.Begin(); i != scratchBuffers_.End(); ++i)
    {
        if (i->reserved_ && i->data_.Get() == buffer)
        {
            i->reserved_ = false;
            return;
        }
    }
    
    LOGWARNING("Reserved scratch buffer " + ToStringHex((unsigned)buffer) + " not found");
}

void Graphics::CleanupScratchBuffers()
{
    for (Vector<ScratchBuffer>::Iterator i = scratchBuffers_.Begin(); i != scratchBuffers_.End(); ++i)
    {
        if (!i->reserved_ && i->size_ > maxScratchBufferRequest_ * 2)
        {
            i->data_ = maxScratchBufferRequest_ > 0 ? new unsigned char[maxScratchBufferRequest_] : 0;
            i->size_ = maxScratchBufferRequest_;
            
            LOGDEBUG("Resized scratch buffer to size " + String(maxScratchBufferRequest_));
        }
    }
    
    maxScratchBufferRequest_ = 0;
}

void Graphics::Release(bool clearGPUObjects, bool closeWindow)
{
    if (!impl_->window_)
        return;
    
    if (clearGPUObjects)
    {
        // Shutting down: release all GPU objects that still exist
        for (Vector<GPUObject*>::Iterator i = gpuObjects_.Begin(); i != gpuObjects_.End(); ++i)
            (*i)->Release();
        gpuObjects_.Clear();
    }
    else
    {
        // We are not shutting down, but recreating the context: mark GPU objects lost
        for (Vector<GPUObject*>::Iterator i = gpuObjects_.Begin(); i != gpuObjects_.End(); ++i)
            (*i)->OnDeviceLost();
    }
    
    CleanupFramebuffers(true);
    depthTextures_.Clear();
    shaderPrograms_.Clear();
    
    if (impl_->context_)
    {
        // Do not log this message if we are exiting
        if (!clearGPUObjects)
            LOGINFO("OpenGL context lost");
        
        MutexLock lock(GetStaticMutex());
        
        SDL_GL_DeleteContext(impl_->context_);
        impl_->context_ = 0;
    }
    
    if (closeWindow)
    {
        MutexLock lock(GetStaticMutex());
        
        SDL_ShowCursor(SDL_TRUE);
        
        // Do not destroy external window except when shutting down
        if (!externalWindow_ || clearGPUObjects)
        {
            SDL_DestroyWindow(impl_->window_);
            impl_->window_ = 0;
        }
    }
}

void Graphics::Restore()
{
    if (!impl_->window_)
        return;
    
    // Ensure first that the context exists
    if (!impl_->context_)
    {
        impl_->context_ = SDL_GL_CreateContext(impl_->window_);
        #ifdef IOS
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&impl_->systemFbo_);
        #endif
        ResetCachedState();
    }
    if (!impl_->context_)
        return;
    
    for (Vector<GPUObject*>::Iterator i = gpuObjects_.Begin(); i != gpuObjects_.End(); ++i)
        (*i)->OnDeviceReset();
}

void Graphics::CleanupRenderSurface(RenderSurface* surface)
{
    if (!surface)
        return;
    
    // Flush pending FBO changes first if any
    CommitFramebuffer();
    
    unsigned currentFbo = impl_->boundFbo_;
    
    // Go through all FBOs and clean up the surface from them
    for (HashMap<unsigned long long, FrameBufferObject>::Iterator i = impl_->frameBuffers_.Begin();
        i != impl_->frameBuffers_.End(); ++i)
    {
        for (unsigned j = 0; j < MAX_RENDERTARGETS; ++j)
        {
            if (i->second_.colorAttachments_[j] == surface)
            {
                if (currentFbo != i->second_.fbo_)
                {
                    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, i->second_.fbo_);
                    currentFbo = i->second_.fbo_;
                }
                glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + j, GL_TEXTURE_2D, 0, 0);
                i->second_.colorAttachments_[j] = 0;
                // Mark drawbuffer bits to need recalculation
                i->second_.drawBuffers_ = M_MAX_UNSIGNED;
            }
        }
        if (i->second_.depthAttachment_ == surface)
        {
            if (currentFbo != i->second_.fbo_)
            {
                glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, i->second_.fbo_);
                currentFbo = i->second_.fbo_;
            }
            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, 0, 0);
            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_TEXTURE_2D, 0, 0);
            i->second_.depthAttachment_ = 0;
        }
    }
    
    // Restore previously bound FBO now if needed
    if (currentFbo != impl_->boundFbo_)
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, impl_->boundFbo_);
}

unsigned Graphics::GetAlphaFormat()
{
    return GL_ALPHA;
}

unsigned Graphics::GetLuminanceFormat()
{
    return GL_LUMINANCE;
}

unsigned Graphics::GetLuminanceAlphaFormat()
{
    return GL_LUMINANCE_ALPHA;
}

unsigned Graphics::GetRGBFormat()
{
    return GL_RGB;
}

unsigned Graphics::GetRGBAFormat()
{
    return GL_RGBA;
}

unsigned Graphics::GetRGBA16Format()
{
    #ifndef GL_ES_VERSION_2_0
    return GL_RGBA16;
    #else
    return GL_RGBA;
    #endif
}

unsigned Graphics::GetRGBAFloat16Format()
{
    #ifndef GL_ES_VERSION_2_0
    return GL_RGBA16F_ARB;
    #else
    return GL_RGBA;
    #endif
}

unsigned Graphics::GetRGBAFloat32Format()
{
    #ifndef GL_ES_VERSION_2_0
    return GL_RGBA32F_ARB;
    #else
    return GL_RGBA;
    #endif
}

unsigned Graphics::GetRG16Format()
{
    #ifndef GL_ES_VERSION_2_0
    return GL_RG16;
    #else
    return GL_RGBA;
    #endif
}

unsigned Graphics::GetRGFloat16Format()
{
    #ifndef GL_ES_VERSION_2_0
    return GL_RG16F;
    #else
    return GL_RGBA;
    #endif
}

unsigned Graphics::GetRGFloat32Format()
{
    #ifndef GL_ES_VERSION_2_0
    return GL_RG32F;
    #else
    return GL_RGBA;
    #endif
}

unsigned Graphics::GetFloat16Format()
{
    #ifndef GL_ES_VERSION_2_0
    return GL_LUMINANCE16F_ARB;
    #else
    return GL_LUMINANCE;
    #endif
}

unsigned Graphics::GetFloat32Format()
{
    #ifndef GL_ES_VERSION_2_0
    return GL_LUMINANCE32F_ARB;
    #else
    return GL_LUMINANCE;
    #endif
}

unsigned Graphics::GetLinearDepthFormat()
{
    // OpenGL FBO specs state that color attachments must have the same format; therefore must encode linear depth to RGBA
    // manually if not using a readable hardware depth texture
    return GL_RGBA;
}

unsigned Graphics::GetDepthStencilFormat()
{
    #ifndef GL_ES_VERSION_2_0
    return GL_DEPTH24_STENCIL8_EXT;
    #else
    return GL_DEPTH_COMPONENT;
    #endif
}

unsigned Graphics::GetFormat(const String& formatName)
{
    String nameLower = formatName.ToLower().Trimmed();
    
    if (nameLower == "a")
        return GetAlphaFormat();
    if (nameLower == "l")
        return GetLuminanceFormat();
    if (nameLower == "la")
        return GetLuminanceAlphaFormat();
    if (nameLower == "rgb")
        return GetRGBFormat();
    if (nameLower == "rgba")
        return GetRGBAFormat();
    if (nameLower == "rgba16")
        return GetRGBA16Format();
    if (nameLower == "rgba16f")
        return GetRGBAFloat16Format();
    if (nameLower == "rgba32f")
        return GetRGBAFloat32Format();
    if (nameLower == "rg16")
        return GetRG16Format();
    if (nameLower == "rg16f")
        return GetRGFloat16Format();
    if (nameLower == "rg32f")
        return GetRGFloat32Format();
    if (nameLower == "r16f")
        return GetFloat16Format();
    if (nameLower == "r32f" || nameLower == "float")
        return GetFloat32Format();
    if (nameLower == "d24s8")
        return GetDepthStencilFormat();
    
    return GetRGBFormat();
}

void Graphics::CheckFeatureSupport()
{
    // Check supported features: light pre-pass, deferred rendering and hardware depth texture
    lightPrepassSupport_ = false;
    deferredSupport_ = false;
    
    int numSupportedRTs = 1;
    
    #ifndef GL_ES_VERSION_2_0
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS_EXT, &numSupportedRTs);
    
    // If hardware depth is not supported, must support 2 rendertargets for light pre-pass, and 4 for deferred
    if (numSupportedRTs >= 2)
        lightPrepassSupport_ = true;
    if (numSupportedRTs >= 4)
        deferredSupport_ = true;
    #else
    if (!CheckExtension("GL_OES_depth_texture"))
    {
        shadowMapFormat_ = 0;
        hiresShadowMapFormat_ = 0;
    }
    else
    {
        shadowMapFormat_ = GL_DEPTH_COMPONENT;
        hiresShadowMapFormat_ = 0;
    }
    #endif
}

void Graphics::CommitFramebuffer()
{
    if (!impl_->fboDirty_)
        return;
    
    impl_->fboDirty_ = false;
    
    // First check if no framebuffer is needed. In that case simply return to backbuffer rendering
    bool noFbo = !depthStencil_;
    if (noFbo)
    {
        for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
        {
            if (renderTargets_[i])
            {
                noFbo = false;
                break;
            }
        }
    }
    
    if (noFbo)
    {
        if (impl_->boundFbo_ != impl_->systemFbo_)
        {
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, impl_->systemFbo_);
            impl_->boundFbo_ = impl_->systemFbo_;
        }
        
        return;
    }
    
    // Search for a new framebuffer based on format & size, or create new
    IntVector2 rtSize = Graphics::GetRenderTargetDimensions();
    unsigned format = 0;
    if (renderTargets_[0])
        format = renderTargets_[0]->GetParentTexture()->GetFormat();
    else if (depthStencil_)
        format = depthStencil_->GetParentTexture()->GetFormat();
    
    unsigned long long fboKey = (rtSize.x_ << 16 | rtSize.y_) | (((unsigned long long)format) << 32);
    
    HashMap<unsigned long long, FrameBufferObject>::Iterator i = impl_->frameBuffers_.Find(fboKey);
    if (i == impl_->frameBuffers_.End())
    {
        FrameBufferObject newFbo;
        glGenFramebuffersEXT(1, &newFbo.fbo_);
        i = impl_->frameBuffers_.Insert(MakePair(fboKey, newFbo));
    }
    
    i->second_.useTimer_.Reset();
    
    if (impl_->boundFbo_ != i->second_.fbo_)
    {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, i->second_.fbo_);
        impl_->boundFbo_ = i->second_.fbo_;
    }
    
    #ifndef GL_ES_VERSION_2_0
    // Setup readbuffers & drawbuffers if needed
    if (i->second_.readBuffers_ != GL_NONE)
    {
        glReadBuffer(GL_NONE);
        i->second_.readBuffers_ = GL_NONE;
    }
    
    // Calculate the bit combination of non-zero color rendertargets to first check if the combination changed
    unsigned newDrawBuffers = 0;
    for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
    {
        if (renderTargets_[i])
            newDrawBuffers |= 1 << i;
    }
    
    if (newDrawBuffers != i->second_.drawBuffers_)
    {
        // Check for no color rendertargets (depth rendering only)
        if (!newDrawBuffers)
            glDrawBuffer(GL_NONE);
        else
        {
            int drawBufferIds[4];
            unsigned drawBufferCount = 0;
            
            for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
            {
                if (renderTargets_[i])
                    drawBufferIds[drawBufferCount++] = GL_COLOR_ATTACHMENT0_EXT + i;
            }
            glDrawBuffers(drawBufferCount, (const GLenum*)drawBufferIds);
        }
        
        i->second_.drawBuffers_ = newDrawBuffers;
    }
    #endif
    
    for (unsigned j = 0; j < MAX_RENDERTARGETS; ++j)
    {
        if (renderTargets_[j])
        {
            Texture* texture = renderTargets_[j]->GetParentTexture();
            
            // If texture's parameters are dirty, update before attaching
            if (texture->GetParametersDirty())
            {
                SetTextureForUpdate(texture);
                texture->UpdateParameters();
                SetTexture(0, 0);
            }
            
            if (i->second_.colorAttachments_[j] != renderTargets_[j])
            {
                glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + j, renderTargets_[j]->GetTarget(),
                    texture->GetGPUObject(), 0);
                i->second_.colorAttachments_[j] = renderTargets_[j];
            }
        }
        else
        {
            if (i->second_.colorAttachments_[j])
            {
                glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + j, GL_TEXTURE_2D, 0, 0);
                i->second_.colorAttachments_[j] = 0;
            }
        }
    }
    
    if (depthStencil_)
    {
        // Bind either a renderbuffer or a depth texture, depending on what is available
        Texture* texture = depthStencil_->GetParentTexture();
        #ifndef GL_ES_VERSION_2_0
        bool hasStencil = texture->GetFormat() == GetDepthStencilFormat();
        #else
        bool hasStencil = false;
        #endif
        unsigned renderBufferID = depthStencil_->GetRenderBuffer();
        if (!renderBufferID)
        {
            // If texture's parameters are dirty, update before attaching
            if (texture->GetParametersDirty())
            {
                SetTextureForUpdate(texture);
                texture->UpdateParameters();
                SetTexture(0, 0);
            }
            
            if (i->second_.depthAttachment_ != depthStencil_)
            {
                glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, texture->GetGPUObject(), 0);
                if (hasStencil)
                {
                    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_TEXTURE_2D,
                        texture->GetGPUObject(), 0);
                }
                else
                    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_TEXTURE_2D, 0, 0);
                
                i->second_.depthAttachment_ = depthStencil_;
            }
        }
        else
        {
            if (i->second_.depthAttachment_ != depthStencil_)
            {
                glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, renderBufferID);
                if (hasStencil)
                {
                    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT,
                        renderBufferID);
                }
                else
                    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_TEXTURE_2D, 0, 0);
                
                i->second_.depthAttachment_ = depthStencil_;
            }
        }
    }
    else
    {
        if (i->second_.depthAttachment_)
        {
            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, 0, 0);
            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_TEXTURE_2D, 0, 0);
            i->second_.depthAttachment_ = 0;
        }
    }
}

bool Graphics::CheckFramebuffer()
{
    return glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT;
}

void Graphics::CleanupFramebuffers(bool contextLost)
{
    if (!contextLost)
    {
        for (HashMap<unsigned long long, FrameBufferObject>::Iterator i = impl_->frameBuffers_.Begin();
            i != impl_->frameBuffers_.End();)
        {
            HashMap<unsigned long long, FrameBufferObject>::Iterator current = i++;
            if (current->second_.fbo_ != impl_->boundFbo_ && current->second_.useTimer_.GetMSec(false) >
                MAX_FRAMEBUFFER_AGE)
            {
                glDeleteFramebuffersEXT(1, &current->second_.fbo_);
                impl_->frameBuffers_.Erase(current);
            }
        }
    }
    else
    {
        impl_->boundFbo_ = 0;
        impl_->frameBuffers_.Clear();
    }
}

void Graphics::ResetCachedState()
{
    for (unsigned i = 0; i < MAX_VERTEX_STREAMS; ++i)
    {
        vertexBuffers_[i] = 0;
        elementMasks_[i] = 0;
    }
    
    for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
    {
        textures_[i] = 0;
        textureTypes_[i] = 0;
    }
    
    for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
        renderTargets_[i] = 0;
    
    depthStencil_ = 0;
    viewTexture_ = 0;
    viewport_ = IntRect(0, 0, 0, 0);
    indexBuffer_ = 0;
    vertexShader_ = 0;
    pixelShader_ = 0;
    shaderProgram_ = 0;
    blendMode_ = BLEND_REPLACE;
    textureAnisotropy_ = 1;
    colorWrite_ = true;
    cullMode_ = CULL_NONE;
    constantDepthBias_ = 0.0f;
    slopeScaledDepthBias_ = 0.0f;
    depthTestMode_ = CMP_ALWAYS;
    depthWrite_ = false;
    fillMode_ = FILL_SOLID;
    scissorTest_ = false;
    scissorRect_ = IntRect::ZERO;
    stencilTest_ = false;
    stencilTestMode_ = CMP_ALWAYS;
    stencilPass_ = OP_KEEP;
    stencilFail_ = OP_KEEP;
    stencilZFail_ = OP_KEEP;
    stencilRef_ = 0;
    stencilCompareMask_ = M_MAX_UNSIGNED;
    stencilWriteMask_ = M_MAX_UNSIGNED;
    impl_->activeTexture_ = 0;
    impl_->enabledAttributes_ = 0;
    impl_->boundFbo_ = impl_->systemFbo_;
    
    // Set initial state to match Direct3D
    if (impl_->context_)
    {
        glEnable(GL_DEPTH_TEST);
        SetCullMode(CULL_CCW);
        SetDepthTest(CMP_LESSEQUAL);
        SetDepthWrite(true);
    }
}

void Graphics::SetTextureUnitMappings()
{
    textureUnits_["DiffMap"] = TU_DIFFUSE;
    textureUnits_["DiffCubeMap"] = TU_DIFFUSE;
    textureUnits_["NormalMap"] = TU_NORMAL;
    textureUnits_["SpecMap"] = TU_SPECULAR;
    textureUnits_["EmissiveMap"] = TU_EMISSIVE;
    textureUnits_["EnvMap"] = TU_ENVIRONMENT;
    textureUnits_["EnvCubeMap"] = TU_ENVIRONMENT;
    textureUnits_["LightRampMap"] = TU_LIGHTRAMP;
    textureUnits_["LightSpotMap"] = TU_LIGHTSHAPE;
    textureUnits_["LightCubeMap"]  = TU_LIGHTSHAPE;
    textureUnits_["ShadowMap"] = TU_SHADOWMAP;
    textureUnits_["FaceSelectCubeMap"] = TU_FACESELECT;
    textureUnits_["IndirectionCubeMap"] = TU_INDIRECTION;
    textureUnits_["AlbedoBuffer"] = TU_ALBEDOBUFFER;
    textureUnits_["NormalBuffer"] = TU_NORMALBUFFER;
    textureUnits_["DepthBuffer"] = TU_DEPTHBUFFER;
    textureUnits_["LightBuffer"] = TU_LIGHTBUFFER;
}

void RegisterGraphicsLibrary(Context* context)
{
    Animation::RegisterObject(context);
    Material::RegisterObject(context);
    Model::RegisterObject(context);
    Shader::RegisterObject(context);
    Technique::RegisterObject(context);
    Texture2D::RegisterObject(context);
    TextureCube::RegisterObject(context);
    Camera::RegisterObject(context);
    Drawable::RegisterObject(context);
    Light::RegisterObject(context);
    StaticModel::RegisterObject(context);
    Skybox::RegisterObject(context);
    AnimatedModel::RegisterObject(context);
    AnimationController::RegisterObject(context);
    BillboardSet::RegisterObject(context);
    ParticleEmitter::RegisterObject(context);
    CustomGeometry::RegisterObject(context);
    DecalSet::RegisterObject(context);
    Terrain::RegisterObject(context);
    TerrainPatch::RegisterObject(context);
    DebugRenderer::RegisterObject(context);
    Octree::RegisterObject(context);
    Zone::RegisterObject(context);
}

}
