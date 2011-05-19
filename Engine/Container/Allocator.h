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

struct Allocator;
struct AllocatorNode;

/// Allocator memory block
struct Allocator
{
    unsigned nodeSize_;
    unsigned capacity_;
    AllocatorNode* firstFreeNode_;
    Allocator* nextBlock_;
    /// Nodes follow
};

/// Allocator node
struct AllocatorNode
{
    Allocator* parentBlock_;
    AllocatorNode* nextFreeNode_;
    /// Payload follows
};

/// Initialize a fixed allocator with the allocation size and initial capacity
Allocator* AllocatorInitialize(unsigned nodeSize, unsigned initialCapacity = 1);
/// Uninitialize a fixed allocator. Frees all blocks in the chain
void AllocatorUninitialize(Allocator* block);
/// Allocate a node. Reserves a new block if necessary
void* AllocatorGet(Allocator* block);
/// Free a node. Does not free any blocks
void AllocatorFree(void* payload);
