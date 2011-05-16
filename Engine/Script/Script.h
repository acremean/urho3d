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

#include "Object.h"

class Scene;
class ScriptFile;
class ScriptInstance;
class asIObjectType;
class asIScriptContext;
class asIScriptEngine;
class asIScriptModule;

struct asSMessageInfo;

/// Script engine logging mode
enum ScriptLogMode
{
    LOGMODE_IMMEDIATE = 0,
    LOGMODE_RETAINED
};

/// Maximum function/method nesting level
static const unsigned MAX_SCRIPT_NESTING_LEVEL = 32;

/// Script subsystem. Allows execution of AngelScript
class Script : public Object
{
    OBJECT(Script);
    
public:
    /// Construct
    Script(Context* context);
    /// Destruct. Release the AngelScript engine
    ~Script();
    
    /// Compile and execute a line of script in immediate mode
    bool Execute(const std::string& line);
    /// Perform garbage collection
    void GarbageCollect(bool fullCycle);
    /// Set immediate mode script file
    void SetDefaultScriptFile(ScriptFile* file);
    /// Set immediate mode scene
    void SetDefaultScene(Scene* scene);
    /// Set script engine logging mode, immediate is default
    void SetLogMode(ScriptLogMode mode);
    /// Clear retained mode log messages
    void ClearLogMessages();
    /// Print the whole script API (all registered classes, methods and properties) to the log
    void DumpAPI();
    /// Log a message from the script engine
    void MessageCallback(const asSMessageInfo* msg);
    /// Handle a script exception
    void ExceptionCallback(asIScriptContext* context);
    
    /// Return the AngelScript engine
    asIScriptEngine* GetScriptEngine() const { return scriptEngine_; }
    /// Return immediate execution script context
    asIScriptContext* GetImmediateContext() const { return immediateContext_; }
    /// Return a script function/method execution context for the current execution nesting level
    asIScriptContext* GetScriptFileContext() const;
    /// Return immediate mode script file
    ScriptFile* GetDefaultScriptFile() const;
    /// Return immediate mode scene
    Scene* GetDefaultScene() const;
    /// Query for an inbuilt object type by constant declaration. Can not be used for script types
    asIObjectType* GetObjectType(const char* declaration);
    /// Return logging mode
    ScriptLogMode GetLogMode() const { return logMode_; }
    /// Return retained mode log messages
    const std::string& GetLogMessages() const { return logMessages_; }
    
    /// Increase script nesting level
    void IncScriptNestingLevel() { ++scriptNestingLevel_; }
    /// Decrease script nesting level
    void DecScriptNestingLevel() { --scriptNestingLevel_; }
    /// Return current script nesting level
    unsigned GetScriptNestingLevel() { return scriptNestingLevel_; }
    /// Return script module to script file map
    std::map<asIScriptModule*, ScriptFile*>& GetModuleMap() { return moduleMap_; }
    /// Return script object to script instance map
    std::map<void*, ScriptInstance*>& GetObjectMap() { return objectMap_; }
    
private:
    /// Output a sanitated row of script API
    void OutputAPIRow(const std::string& row, bool removeReference = false);
    
    /// AngelScript engine
    asIScriptEngine* scriptEngine_;
    /// Immediate execution script context
    asIScriptContext* immediateContext_;
    /// Immediate execution script file
    WeakPtr<ScriptFile> defaultScriptFile_;
    /// Immediate execution scene
    WeakPtr<Scene> defaultScene_;
    /// Script function/method execution contexts
    std::vector<asIScriptContext*> scriptFileContexts_;
    /// Map of script modules to script files
    std::map<asIScriptModule*, ScriptFile*> moduleMap_;
    /// Map of script objects to script instance components
    std::map<void*, ScriptInstance*> objectMap_;
    /// Search cache for inbuilt object types (constant declaration)
    std::map<const char*, asIObjectType*> objectTypes_;
    /// Script engine logging mode
    ScriptLogMode logMode_;
    /// Retained mode log messages
    std::string logMessages_;
    /// Current script execution nesting level
    unsigned scriptNestingLevel_;
};

/// Register Script library objects
void RegisterScriptLibrary(Context* context);