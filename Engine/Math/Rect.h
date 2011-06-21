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

#include "Vector2.h"

/// Two-dimensional bounding rectangle
class Rect
{
public:
    /// Construct an undefined rect
    Rect() :
        defined_(false)
    {
    }
    
    /// Copy-construct from another rect
    Rect(const Rect& rect) :
        min_(rect.min_),
        max_(rect.max_),
        defined_(true)
    {
    }
    
    /// Construct from minimum and maximum vectors
    Rect(const Vector2& min, const Vector2& max) :
        min_(min),
        max_(max),
        defined_(true)
    {
    }
    
    /// Construct from coordinates
    Rect(float left, float top, float right, float bottom) :
        min_(left, top),
        max_(right, bottom),
        defined_(true)
    {
    }
    
    /// Assign from another rect
    Rect& operator = (const Rect& rhs)
    {
        min_ = rhs.min_;
        max_ = rhs.max_;
        defined_ = true;
        
        return *this;
    }
    
    /// Test for equality with another rect
    bool operator == (const Rect& rhs) const { return min_ == rhs.min_ && max_ == rhs.max_; }
    /// Test for inequality with another rect
    bool operator != (const Rect& rhs) const { return min_ != rhs.min_ || max_ != rhs.max_; }
    
    /// Define from minimum and maximum vectors
    void Define(const Vector2& min, const Vector2& max)
    {
        min_ = min;
        max_ = max;
        defined_ = true;
    }
    
    /// Define from a point
    void Define(const Vector2& point)
    {
        min_ = max_ = point;
        defined_ = true;
    }
    
    /// Merge a point
    void Merge(const Vector2& point)
    {
        if (!defined_)
        {
            min_ = max_ = point;
            defined_ = true;
        }
        
        if (point.x_ < min_.x_)
            min_.x_ = point.x_;
        if (point.x_ > max_.x_)
            max_.x_ = point.x_;
        if (point.y_ < min_.y_)
            min_.y_ = point.y_;
        if (point.y_ > max_.y_)
            max_.y_ = point.y_;
    }
    
    /// Merge a rect
    void Merge(const Rect& rect)
    {
        if (!defined_)
        {
            min_ = rect.min_;
            max_ = rect.max_;
            defined_ = true;
        }
        
        if (rect.min_.x_ < min_.x_) min_.x_ = rect.min_.x_;
        if (rect.min_.y_ < min_.y_) min_.y_ = rect.min_.y_;
        if (rect.max_.x_ > max_.x_) max_.x_ = rect.max_.x_;
        if (rect.max_.y_ > max_.y_) max_.y_ = rect.max_.y_;
    }
    
    /// Return float data
    const void* GetData() const { return &min_.x_; }
    /// Return as string
    String ToString() const;
    
    /// Minimum vector
    Vector2 min_;
    /// Maximum vector
    Vector2 max_;
    /// Defined flag
    bool defined_;
    
    /// Rect in the range (-1, -1) - (1, 1)
    static const Rect FULL;
    /// Rect in the range (0, 0) - (1, 1)
    static const Rect POSITIVE;
    /// Zero-sized rect
    static const Rect ZERO;
};

/// Two-dimensional bounding rectangle with integer values
class IntRect
{
public:
    /// Construct an undefined rect
    IntRect()
    {
    }
    
    /// Construct from coordinates
    IntRect(int left, int top, int right, int bottom) :
        left_(left),
        top_(top),
        right_(right),
        bottom_(bottom)
    {
    }
    
    /// Test for equality with another rect
    bool operator == (const IntRect& rhs) const
    {
        return left_ == rhs.left_ && top_ == rhs.top_ && right_ == rhs.right_ && bottom_ == rhs.bottom_;
    }
    
    /// Test for inequality with another rect
    bool operator != (const IntRect& rhs) const
    {
        return left_ != rhs.left_ || top_ != rhs.top_ || right_ != rhs.right_ || bottom_ != rhs.bottom_;
    }
    
    /// Return integer data
    const int* GetData() const { return &left_; }
    /// Return as string
    String ToString() const;
    
    /// Left coordinate
    int left_;
    /// Top coordinate
    int top_;
    /// Right coordinate
    int right_;
    /// Bottom coordinate
    int bottom_;
    
    /// Zero-sized rect
    static const IntRect ZERO;
};
