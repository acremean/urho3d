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
#include "StringUtils.h"
#include "Variant.h"

#include <cstring>

#include "DebugNew.h"

const Variant Variant::EMPTY;
const std::string Variant::emptyString;
const std::vector<unsigned char> Variant::emptyBuffer;
const ResourceRef Variant::emptyResourceRef;
const ResourceRefList Variant::emptyResourceRefList;
const VariantMap Variant::emptyVariantMap;
const VariantVector Variant::emptyVariantVector;

static const std::string typeNames[] =
{
    "None",
    "Int",
    "Bool",
    "Float",
    "Vector2",
    "Vector3",
    "Vector4",
    "Quaternion",
    "Color",
    "String",
    "Buffer",
    "Pointer",
    "ResourceRef",
    "ResourceRefList",
    "VariantVector",
    "VariantMap"
};

Variant& Variant::operator = (const Variant& rhs)
{
    SetType(rhs.GetType());
    
    switch (type_)
    {
    case VAR_STRING:
        *(reinterpret_cast<std::string*>(value_.ptr_)) = *(reinterpret_cast<const std::string*>(rhs.value_.ptr_));
        break;
        
    case VAR_BUFFER:
        *(reinterpret_cast<std::vector<unsigned char>*>(value_.ptr_)) = *(reinterpret_cast<const std::vector<unsigned char>*>(rhs.value_.ptr_));
        break;
    
    case VAR_RESOURCEREF:
        *(reinterpret_cast<ResourceRef*>(value_.ptr_)) = *(reinterpret_cast<ResourceRef*>(rhs.value_.ptr_));
        break;
        
    case VAR_RESOURCEREFLIST:
        *(reinterpret_cast<ResourceRefList*>(value_.ptr_)) = *(reinterpret_cast<ResourceRefList*>(rhs.value_.ptr_));
        break;
        
    case VAR_VARIANTVECTOR:
        *(reinterpret_cast<VariantVector*>(value_.ptr_)) = *(reinterpret_cast<VariantVector*>(rhs.value_.ptr_));
        break;
        
    case VAR_VARIANTMAP:
        *(reinterpret_cast<VariantMap*>(value_.ptr_)) = *(reinterpret_cast<VariantMap*>(rhs.value_.ptr_));
        break;
        
    default:
        value_ = rhs.value_;
        break;
    }
    
    return *this;
}

bool Variant::operator == (const Variant& rhs) const
{
    if (type_ != rhs.type_)
        return false;
    
    switch (type_)
    {
    case VAR_INT:
        return value_.int_ == rhs.value_.int_;
        
    case VAR_BOOL:
        return value_.bool_ == rhs.value_.bool_;
        
    case VAR_FLOAT:
        return value_.float_ == rhs.value_.float_;
        
    case VAR_VECTOR2:
        return *(reinterpret_cast<const Vector2*>(&value_)) == *(reinterpret_cast<const Vector2*>(&rhs.value_));
        
    case VAR_VECTOR3:
        return *(reinterpret_cast<const Vector3*>(&value_)) == *(reinterpret_cast<const Vector3*>(&rhs.value_));
        
    case VAR_VECTOR4:
        return *(reinterpret_cast<const Vector4*>(&value_)) == *(reinterpret_cast<const Vector4*>(&rhs.value_));
        
    case VAR_QUATERNION:
        return *(reinterpret_cast<const Quaternion*>(&value_)) == *(reinterpret_cast<const Quaternion*>(&rhs.value_));
        
    case VAR_COLOR:
        return *(reinterpret_cast<const Color*>(&value_)) == *(reinterpret_cast<const Color*>(&rhs.value_));
        
    case VAR_STRING:
        return *(reinterpret_cast<const std::string*>(value_.ptr_)) == *(reinterpret_cast<const std::string*>(rhs.value_.ptr_));
        
    case VAR_BUFFER:
        return *(reinterpret_cast<const std::vector<unsigned char>*>(value_.ptr_)) == *(reinterpret_cast<const std::vector<unsigned char>*>(rhs.value_.ptr_));
        
    case VAR_PTR:
        return value_.ptr_ == rhs.value_.ptr_;
        
    case VAR_RESOURCEREF:
        return *(reinterpret_cast<const ResourceRef*>(value_.ptr_)) == *(reinterpret_cast<const ResourceRef*>(rhs.value_.ptr_));
        
    case VAR_RESOURCEREFLIST:
        return *(reinterpret_cast<const ResourceRefList*>(value_.ptr_)) == *(reinterpret_cast<const ResourceRefList*>(rhs.value_.ptr_));
        
    case VAR_VARIANTVECTOR:
        return *(reinterpret_cast<const VariantVector*>(value_.ptr_)) == *(reinterpret_cast<const VariantVector*>(rhs.value_.ptr_));
        
    case VAR_VARIANTMAP:
        return *(reinterpret_cast<const VariantMap*>(value_.ptr_)) == *(reinterpret_cast<const VariantMap*>(rhs.value_.ptr_));
    }
    
    return true;
}

void Variant::FromString(const std::string& type, const std::string& value)
{
    std::string typeLower = ToLower(type);
    
    if (typeLower == "none")
        SetType(VAR_NONE);
    else if (typeLower == "int")
        *this = ToInt(value);
    else if (typeLower == "bool")
        *this = ToBool(value);
    else if (typeLower == "float")
        *this = ToFloat(value);
    else if (typeLower == "vector2")
        *this = ToVector2(value);
    else if (typeLower == "vector3")
        *this = ToVector3(value);
    else if (typeLower == "vector4")
        *this = ToVector4(value);
    else if (typeLower == "quaternion")
        *this = ToQuaternion(value);
    else if (typeLower == "color")
        *this = ToColor(value);
    else if (typeLower == "string")
        *this = value;
    else if (typeLower == "buffer")
    {
        SetType(VAR_BUFFER);
        std::vector<unsigned char>& buffer = *(reinterpret_cast<std::vector<unsigned char>*>(value_.ptr_));
        std::vector<std::string> values = Split(value, ' ');
        buffer.resize(values.size());
        for (unsigned i = 0; i < values.size(); ++i)
            buffer[i] = ToInt(values[i]);
    }
    else if (typeLower == "pointer")
    {
        *this = (void*)0;
    }
    else if (typeLower == "objectref")
    {
        std::vector<std::string> values = Split(value, ';');
        if (values.size() == 2)
        {
            SetType(VAR_RESOURCEREF);
            ResourceRef& ref = *(reinterpret_cast<ResourceRef*>(value_.ptr_));
            ref.type_ = ShortStringHash(values[0]);
            ref.id_ = StringHash(values[1]);
        }
    }
    else if (typeLower == "objectreflist")
    {
        std::vector<std::string> values = Split(value, ';');
        if (values.size() >= 1)
        {
            SetType(VAR_RESOURCEREFLIST);
            ResourceRefList& refList = *(reinterpret_cast<ResourceRefList*>(value_.ptr_));
            refList.type_ = ShortStringHash(values[0]);
            refList.ids_.resize(values.size() - 1);
            for (unsigned i = 1; i < values.size(); ++i)
                refList.ids_[i - 1] = StringHash(values[i]);
        }
    }
    else
        SetType(VAR_NONE);
}

void Variant::SetBuffer(const void* data, unsigned size)
{
    if ((size) && (!data))
        size = 0;
    
    SetType(VAR_BUFFER);
    std::vector<unsigned char>& buffer = *(reinterpret_cast<std::vector<unsigned char>*>(value_.ptr_));
    buffer.resize(size);
    if (size)
        memcpy(&buffer[0], data, size);
}

const std::string& Variant::GetTypeName() const
{
    return typeNames[type_];
}

std::string Variant::ToString() const
{
    switch (type_)
    {
    case VAR_INT:
        return ::ToString(value_.int_);
        
    case VAR_BOOL:
        return ::ToString(value_.bool_);
        
    case VAR_FLOAT:
        return ::ToString(value_.float_);
        
    case VAR_VECTOR2:
        return ::ToString(*(reinterpret_cast<const Vector2*>(&value_)));
        
    case VAR_VECTOR3:
        return ::ToString(*(reinterpret_cast<const Vector3*>(&value_)));
        
    case VAR_VECTOR4:
        return ::ToString(*(reinterpret_cast<const Vector4*>(&value_)));
        
    case VAR_QUATERNION:
        return ::ToString(*(reinterpret_cast<const Quaternion*>(&value_)));
        
    case VAR_COLOR:
        return ::ToString(*(reinterpret_cast<const Color*>(&value_)));
        
    case VAR_STRING:
        return *(reinterpret_cast<const std::string*>(value_.ptr_));
        
    case VAR_BUFFER:
        {
            const std::vector<unsigned char>& buffer = *(reinterpret_cast<const std::vector<unsigned char>*>(value_.ptr_));
            std::string ret;
            for (std::vector<unsigned char>::const_iterator i = buffer.begin(); i != buffer.end(); ++i)
            {
                if (i != buffer.begin())
                    ret += " ";
                ret += ::ToString(*i);
            }
            return ret;
        }
        
    case VAR_PTR:
        // Pointer serialization not supported (convert to null)
        return ::ToString((unsigned)0);
        
    case VAR_RESOURCEREF:
    case VAR_RESOURCEREFLIST:
    case VAR_VARIANTVECTOR:
    case VAR_VARIANTMAP:
        // Reference string serialization requires hash-to-name mapping from the context & subsystems. Can not support here
        // Also variant map or vector string serialization is not supported. XML or binary save should be used instead
        return std::string();
    }
    
    return std::string();
}

void Variant::SetType(VariantType newType)
{
    if (type_ == newType)
        return;
    
    switch (type_)
    {
    case VAR_STRING:
        delete reinterpret_cast<std::string*>(value_.ptr_);
        break;
        
    case VAR_BUFFER:
        delete reinterpret_cast<std::vector<unsigned char>*>(value_.ptr_);
        break;
        
    case VAR_RESOURCEREF:
        delete reinterpret_cast<ResourceRef*>(value_.ptr_);
        break;
        
    case VAR_RESOURCEREFLIST:
        delete reinterpret_cast<ResourceRefList*>(value_.ptr_);
        break;
        
    case VAR_VARIANTVECTOR:
        delete reinterpret_cast<VariantVector*>(value_.ptr_);
        break;
        
    case VAR_VARIANTMAP:
        delete reinterpret_cast<VariantMap*>(value_.ptr_);
        break;
    }
    
    type_ = newType;
    
    switch (type_)
    {
    case VAR_STRING:
        *reinterpret_cast<std::string**>(&value_.ptr_) = new std::string();
        break;
        
    case VAR_BUFFER:
        *reinterpret_cast<std::vector<unsigned char>**>(&value_.ptr_) = new std::vector<unsigned char>();
        break;
        
    case VAR_RESOURCEREF:
        *reinterpret_cast<ResourceRef**>(&value_.ptr_) = new ResourceRef();
        break;
        
    case VAR_RESOURCEREFLIST:
        *reinterpret_cast<ResourceRefList**>(&value_.ptr_) = new ResourceRefList();
        break;
        
    case VAR_VARIANTVECTOR:
        *reinterpret_cast<VariantVector**>(&value_.ptr_) = new VariantVector();
        break;
        
    case VAR_VARIANTMAP:
        *reinterpret_cast<VariantMap**>(&value_.ptr_) = new VariantMap();
        break;
    }
}

template<> int Variant::Get<int>() const
{
    return GetInt();
}

template<> unsigned Variant::Get<unsigned>() const
{
    return GetUInt();
}

template<> StringHash Variant::Get<StringHash>() const
{
    return GetStringHash();
}

template<> ShortStringHash Variant::Get<ShortStringHash>() const
{
    return GetShortStringHash();
}

template<> bool Variant::Get<bool>() const
{
    return GetBool();
}

template<> float Variant::Get<float>() const
{
    return GetFloat();
}

template<> const Vector2& Variant::Get<const Vector2&>() const
{
    return GetVector2();
}

template<> const Vector3& Variant::Get<const Vector3&>() const
{
    return GetVector3();
}

template<> const Vector4& Variant::Get<const Vector4&>() const
{
    return GetVector4();
}

template<> const Quaternion& Variant::Get<const Quaternion&>() const
{
    return GetQuaternion();
}

template<> const Color& Variant::Get<const Color&>() const
{
    return GetColor();
}

template<> const std::string& Variant::Get<const std::string&>() const
{
    return GetString();
}

template<> const std::vector<unsigned char>& Variant::Get<const std::vector<unsigned char>& >() const
{
    return GetBuffer();
}

template<> void* Variant::Get<void*>() const
{
    return GetPtr();
}

template<> const ResourceRef& Variant::Get<const ResourceRef&>() const
{
    return GetResourceRef();
}

template<> const ResourceRefList& Variant::Get<const ResourceRefList&>() const
{
    return GetResourceRefList();
}

template<> const VariantVector& Variant::Get<const VariantVector&>() const
{
    return GetVariantVector();
}

template<> const VariantMap& Variant::Get<const VariantMap&>() const
{
    return GetVariantMap();
}

template<> Vector2 Variant::Get<Vector2>() const
{
    return GetVector2();
}

template<> Vector3 Variant::Get<Vector3>() const
{
    return GetVector3();
}

template<> Vector4 Variant::Get<Vector4>() const
{
    return GetVector4();
}

template<> Quaternion Variant::Get<Quaternion>() const
{
    return GetQuaternion();
}

template<> Color Variant::Get<Color>() const
{
    return GetColor();
}

template<> std::string Variant::Get<std::string>() const
{
    return GetString();
}

template<> std::vector<unsigned char> Variant::Get<std::vector<unsigned char> >() const
{
    return GetBuffer();
}

const std::string& Variant::GetTypeName(VariantType type)
{
    return typeNames[type];
}