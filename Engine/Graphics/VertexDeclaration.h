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

#pragma once

#include "RefCounted.h"
#include "GraphicsDefs.h"

#include <d3d9.h>
#include <vector>

class Graphics;
class VertexBuffer;

/// Element in the vertex declaration
struct VertexDeclarationElement
{
    /// Stream index
    unsigned stream_;
    /// Element type
    VertexElement element_;
    /// Element offset
    unsigned offset_;
};

/// Vertex declaration
class VertexDeclaration : public RefCounted
{
public:
    /// Construct with graphics subsystem pointer and vertex element mask
    VertexDeclaration(Graphics* graphics, unsigned elementMask);
    /// Construct with graphics subsystem pointer and vertex buffers to base declaration on
    VertexDeclaration(Graphics* graphics, const std::vector<VertexBuffer*>& buffers, const std::vector<unsigned>& elementMasks);
    /// Construct with graphics subsystem pointer and vertex buffers to base declaration on
    VertexDeclaration(Graphics* graphics, const std::vector<SharedPtr<VertexBuffer> >& buffers, const std::vector<unsigned>& elementMasks);
    /// Destruct
    ~VertexDeclaration();
    
    /// Return Direct3D vertex declaration
    IDirect3DVertexDeclaration9* GetDeclaration() const { return declaration_; }
    
private:
    /// Create declaration
    void Create(Graphics* graphics, const std::vector<VertexDeclarationElement>& elements);
    /// Release declaration
    void Release();
    
    /// Direct3D vertex declaration
    IDirect3DVertexDeclaration9* declaration_;
};
