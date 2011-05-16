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
#include "APITemplates.h"
#include "Input.h"

static void RegisterInputConstants(asIScriptEngine* engine)
{
    engine->RegisterGlobalProperty("const int MOUSEB_LEFT", (void*)&MOUSEB_LEFT);
    engine->RegisterGlobalProperty("const int MOUSEB_RIGHT", (void*)&MOUSEB_RIGHT);
    engine->RegisterGlobalProperty("const int MOUSEB_MIDDLE", (void*)&MOUSEB_MIDDLE);
    engine->RegisterGlobalProperty("const int QUAL_SHIFT", (void*)&QUAL_SHIFT);
    engine->RegisterGlobalProperty("const int QUAL_CTRL", (void*)&QUAL_CTRL);
    engine->RegisterGlobalProperty("const int QUAL_ALT", (void*)&QUAL_ALT);
    engine->RegisterGlobalProperty("const int KEY_BACKSPACE", (void*)&KEY_BACKSPACE);
    engine->RegisterGlobalProperty("const int KEY_TAB", (void*)&KEY_TAB);
    engine->RegisterGlobalProperty("const int KEY_RETURN", (void*)&KEY_RETURN);
    engine->RegisterGlobalProperty("const int KEY_SHIFT", (void*)&KEY_SHIFT);
    engine->RegisterGlobalProperty("const int KEY_CTRL", (void*)&KEY_CTRL);
    engine->RegisterGlobalProperty("const int KEY_ALT", (void*)&KEY_ALT);
    engine->RegisterGlobalProperty("const int KEY_PAUSE", (void*)&KEY_PAUSE);
    engine->RegisterGlobalProperty("const int KEY_CAPSLOCK", (void*)&KEY_CAPSLOCK);
    engine->RegisterGlobalProperty("const int KEY_ESC", (void*)&KEY_ESC);
    engine->RegisterGlobalProperty("const int KEY_SPACE", (void*)&KEY_SPACE);
    engine->RegisterGlobalProperty("const int KEY_PAGEUP", (void*)&KEY_PAGEUP);
    engine->RegisterGlobalProperty("const int KEY_PAGEDOWN", (void*)&KEY_PAGEDOWN);
    engine->RegisterGlobalProperty("const int KEY_END", (void*)&KEY_END);
    engine->RegisterGlobalProperty("const int KEY_HOME", (void*)&KEY_HOME);
    engine->RegisterGlobalProperty("const int KEY_LEFT", (void*)&KEY_LEFT);
    engine->RegisterGlobalProperty("const int KEY_UP", (void*)&KEY_UP);
    engine->RegisterGlobalProperty("const int KEY_RIGHT", (void*)&KEY_RIGHT);
    engine->RegisterGlobalProperty("const int KEY_DOWN", (void*)&KEY_DOWN);
    engine->RegisterGlobalProperty("const int KEY_PRINTSCREEN", (void*)&KEY_PRINTSCREEN);
    engine->RegisterGlobalProperty("const int KEY_INSERT", (void*)&KEY_INSERT);
    engine->RegisterGlobalProperty("const int KEY_DELETE", (void*)&KEY_DELETE);
    engine->RegisterGlobalProperty("const int KEY_LWIN", (void*)&KEY_LWIN);
    engine->RegisterGlobalProperty("const int KEY_RWIN", (void*)&KEY_RWIN);
    engine->RegisterGlobalProperty("const int KEY_APPS", (void*)&KEY_APPS);
    engine->RegisterGlobalProperty("const int KEY_NUMPAD0", (void*)&KEY_NUMPAD0);
    engine->RegisterGlobalProperty("const int KEY_NUMPAD1", (void*)&KEY_NUMPAD1);
    engine->RegisterGlobalProperty("const int KEY_NUMPAD2", (void*)&KEY_NUMPAD2);
    engine->RegisterGlobalProperty("const int KEY_NUMPAD3", (void*)&KEY_NUMPAD3);
    engine->RegisterGlobalProperty("const int KEY_NUMPAD4", (void*)&KEY_NUMPAD4);
    engine->RegisterGlobalProperty("const int KEY_NUMPAD5", (void*)&KEY_NUMPAD5);
    engine->RegisterGlobalProperty("const int KEY_NUMPAD6", (void*)&KEY_NUMPAD6);
    engine->RegisterGlobalProperty("const int KEY_NUMPAD7", (void*)&KEY_NUMPAD7);
    engine->RegisterGlobalProperty("const int KEY_NUMPAD8", (void*)&KEY_NUMPAD8);
    engine->RegisterGlobalProperty("const int KEY_NUMPAD9", (void*)&KEY_NUMPAD9);
    engine->RegisterGlobalProperty("const int KEY_MULTIPLY", (void*)&KEY_MULTIPLY);
    engine->RegisterGlobalProperty("const int KEY_ADD", (void*)&KEY_ADD);
    engine->RegisterGlobalProperty("const int KEY_SUBTRACT", (void*)&KEY_SUBTRACT);
    engine->RegisterGlobalProperty("const int KEY_DECIMAL", (void*)&KEY_DECIMAL);
    engine->RegisterGlobalProperty("const int KEY_DIVIDE", (void*)&KEY_DIVIDE);
    engine->RegisterGlobalProperty("const int KEY_F1", (void*)&KEY_F1);
    engine->RegisterGlobalProperty("const int KEY_F2", (void*)&KEY_F2);
    engine->RegisterGlobalProperty("const int KEY_F3", (void*)&KEY_F3);
    engine->RegisterGlobalProperty("const int KEY_F4", (void*)&KEY_F4);
    engine->RegisterGlobalProperty("const int KEY_F5", (void*)&KEY_F5);
    engine->RegisterGlobalProperty("const int KEY_F6", (void*)&KEY_F6);
    engine->RegisterGlobalProperty("const int KEY_F7", (void*)&KEY_F7);
    engine->RegisterGlobalProperty("const int KEY_F8", (void*)&KEY_F8);
    engine->RegisterGlobalProperty("const int KEY_F9", (void*)&KEY_F9);
    engine->RegisterGlobalProperty("const int KEY_F10", (void*)&KEY_F10);
    engine->RegisterGlobalProperty("const int KEY_F11", (void*)&KEY_F11);
    engine->RegisterGlobalProperty("const int KEY_F12", (void*)&KEY_F12);
    engine->RegisterGlobalProperty("const int KEY_F13", (void*)&KEY_F13);
    engine->RegisterGlobalProperty("const int KEY_F14", (void*)&KEY_F14);
    engine->RegisterGlobalProperty("const int KEY_F15", (void*)&KEY_F15);
    engine->RegisterGlobalProperty("const int KEY_F16", (void*)&KEY_F16);
    engine->RegisterGlobalProperty("const int KEY_F17", (void*)&KEY_F17);
    engine->RegisterGlobalProperty("const int KEY_F18", (void*)&KEY_F18);
    engine->RegisterGlobalProperty("const int KEY_F19", (void*)&KEY_F19);
    engine->RegisterGlobalProperty("const int KEY_F20", (void*)&KEY_F20);
    engine->RegisterGlobalProperty("const int KEY_F21", (void*)&KEY_F21);
    engine->RegisterGlobalProperty("const int KEY_F22", (void*)&KEY_F22);
    engine->RegisterGlobalProperty("const int KEY_F23", (void*)&KEY_F23);
    engine->RegisterGlobalProperty("const int KEY_F24", (void*)&KEY_F24);
    engine->RegisterGlobalProperty("const int KEY_NUMLOCK", (void*)&KEY_NUMLOCK);
    engine->RegisterGlobalProperty("const int KEY_SCROLLLOCK", (void*)&KEY_SCROLLLOCK);
    engine->RegisterGlobalProperty("const int KEY_LSHIFT", (void*)&KEY_LSHIFT);
    engine->RegisterGlobalProperty("const int KEY_RSHIFT", (void*)&KEY_RSHIFT);
    engine->RegisterGlobalProperty("const int KEY_LCTRL", (void*)&KEY_LCTRL);
    engine->RegisterGlobalProperty("const int KEY_RCTRL", (void*)&KEY_RCTRL);
    engine->RegisterGlobalProperty("const int KEY_LALT", (void*)&KEY_LALT);
    engine->RegisterGlobalProperty("const int KEY_RALT", (void*)&KEY_RALT);
    engine->RegisterGlobalProperty("const int KEY_OEM_1", (void*)&KEY_OEM_1);
    engine->RegisterGlobalProperty("const int KEY_OEM_PLUS", (void*)&KEY_OEM_PLUS);
    engine->RegisterGlobalProperty("const int KEY_OEM_COMMA", (void*)&KEY_OEM_COMMA);
    engine->RegisterGlobalProperty("const int KEY_OEM_MINUS", (void*)&KEY_OEM_MINUS);
    engine->RegisterGlobalProperty("const int KEY_OEM_PERIOD", (void*)&KEY_OEM_PERIOD);
    engine->RegisterGlobalProperty("const int KEY_OEM_2", (void*)&KEY_OEM_2);
    engine->RegisterGlobalProperty("const int KEY_OEM_3", (void*)&KEY_OEM_3);
    engine->RegisterGlobalProperty("const int KEY_OEM_4", (void*)&KEY_OEM_4);
    engine->RegisterGlobalProperty("const int KEY_OEM_5", (void*)&KEY_OEM_5);
    engine->RegisterGlobalProperty("const int KEY_OEM_6", (void*)&KEY_OEM_6);
    engine->RegisterGlobalProperty("const int KEY_OEM_7", (void*)&KEY_OEM_7);
    engine->RegisterGlobalProperty("const int KEY_OEM_8", (void*)&KEY_OEM_8);
}

static Input* GetInput()
{
    return GetScriptContext()->GetSubsystem<Input>();
}

static void RegisterInput(asIScriptEngine* engine)
{
    RegisterObject<Input>(engine, "Input");
    engine->RegisterObjectMethod("Input", "void SetMousePosition(int, int)", asMETHODPR(Input, SetMousePosition, (int, int), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("Input", "void SuppressNextChar()", asMETHOD(Input, SuppressNextChar), asCALL_THISCALL);
    engine->RegisterObjectMethod("Input", "void set_clipCursor(bool)", asMETHOD(Input, SetClipCursor), asCALL_THISCALL);
    engine->RegisterObjectMethod("Input", "bool get_clipCursor() const", asMETHOD(Input, GetClipCursor), asCALL_THISCALL);
    engine->RegisterObjectMethod("Input", "void set_toggleFullscreen(bool)", asMETHOD(Input, SetToggleFullscreen), asCALL_THISCALL);
    engine->RegisterObjectMethod("Input", "bool get_toggleFullscreen() const", asMETHOD(Input, GetToggleFullscreen), asCALL_THISCALL);
    engine->RegisterObjectMethod("Input", "void set_mousePosition(const IntVector2&in)", asMETHODPR(Input, SetMousePosition, (const IntVector2&), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("Input", "IntVector2 get_mousePosition() const", asMETHOD(Input, GetMousePosition), asCALL_THISCALL);
    engine->RegisterObjectMethod("Input", "bool get_keyDown(int) const", asMETHOD(Input, GetKeyDown), asCALL_THISCALL);
    engine->RegisterObjectMethod("Input", "bool get_keyPress(int) const", asMETHOD(Input, GetKeyPress), asCALL_THISCALL);
    engine->RegisterObjectMethod("Input", "bool get_mouseButtonDown(int) const", asMETHOD(Input, GetMouseButtonDown), asCALL_THISCALL);
    engine->RegisterObjectMethod("Input", "bool get_mouseButtonPress(int) const", asMETHOD(Input, GetMouseButtonPress), asCALL_THISCALL);
    engine->RegisterObjectMethod("Input", "bool get_qualifierDown(int) const", asMETHOD(Input, GetQualifierDown), asCALL_THISCALL);
    engine->RegisterObjectMethod("Input", "bool get_qualifierPress(int) const", asMETHOD(Input, GetQualifierPress), asCALL_THISCALL);
    engine->RegisterObjectMethod("Input", "int get_qualifiers() const", asMETHOD(Input, GetQualifiers), asCALL_THISCALL);
    engine->RegisterObjectMethod("Input", "const IntVector2& get_mouseMove() const", asMETHOD(Input, GetMouseMove), asCALL_THISCALL);
    engine->RegisterObjectMethod("Input", "int get_mouseMoveX() const", asMETHOD(Input, GetMouseMoveX), asCALL_THISCALL);
    engine->RegisterObjectMethod("Input", "int get_mouseMoveY() const", asMETHOD(Input, GetMouseMoveY), asCALL_THISCALL);
    engine->RegisterObjectMethod("Input", "int get_mouseMoveWheel() const", asMETHOD(Input, GetMouseMoveWheel), asCALL_THISCALL);
    engine->RegisterObjectMethod("Input", "bool get_active() const", asMETHOD(Input, IsActive), asCALL_THISCALL);
    engine->RegisterObjectMethod("Input", "bool get_minimized() const", asMETHOD(Input, IsMinimized), asCALL_THISCALL);
    engine->RegisterGlobalFunction("Input@+ get_input()", asFUNCTION(GetInput), asCALL_CDECL);
}

void RegisterInputAPI(asIScriptEngine* engine)
{
    RegisterInputConstants(engine);
    RegisterInput(engine);
}