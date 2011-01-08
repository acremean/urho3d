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
#include "Mutex.h"

#include <windows.h>

#include "DebugNew.h"

Mutex::Mutex() :
    mCriticalSection(new CRITICAL_SECTION())
{
    InitializeCriticalSection((CRITICAL_SECTION*)mCriticalSection);
}

Mutex::~Mutex()
{
    CRITICAL_SECTION* cs = (CRITICAL_SECTION*)mCriticalSection;
    DeleteCriticalSection(cs);
    delete cs;
    mCriticalSection = 0;
}

void Mutex::acquire()
{
    EnterCriticalSection((CRITICAL_SECTION*)mCriticalSection);
}

void Mutex::release()
{
    LeaveCriticalSection((CRITICAL_SECTION*)mCriticalSection);
}

MutexLock::MutexLock(Mutex& mutex) :
    mMutex(mutex)
{
    mMutex.acquire();
}

MutexLock::~MutexLock()
{
    mMutex.release();
}
