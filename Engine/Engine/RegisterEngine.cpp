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
#include "Client.h"
#include "Connection.h"
#include "DebugHud.h"
#include "Engine.h"
#include "ParticleEmitter.h"
#include "RegisterTemplates.h"
#include "Scene.h"
#include "ScriptInstance.h"
#include "Server.h"

static void SendRemoteEventToAll(const std::string& eventType, const VariantMap& eventData)
{
    Scene* scene = getScriptContextScene();
    if (scene)
        scene->sendRemoteEvent(StringHash(eventType), eventData);
}

static void SendRemoteEvent(const std::string& eventType, const VariantMap& eventData, Connection* receiver)
{
    Scene* scene = getScriptContextScene();
    if (scene)
        scene->sendRemoteEvent(StringHash(eventType), eventData, receiver);
}

static void SendRemoteEventDelayTTL(const std::string& eventType, const VariantMap& eventData, Connection* receiver, float delay, unsigned short timeToLive)
{
    Scene* scene = getScriptContextScene();
    if (scene)
        scene->sendRemoteEvent(StringHash(eventType), eventData, receiver, delay, timeToLive);
}

static void SendRemoteEntityEventToAll(Entity* entity, const std::string& eventType, const VariantMap& eventData)
{
    Scene* scene = getScriptContextScene();
    if (scene)
        scene->sendRemoteEvent(entity, StringHash(eventType), eventData);
}

static void SendRemoteEntityEvent(Entity* entity, const std::string& eventType, const VariantMap& eventData, Connection* receiver)
{
    Scene* scene = getScriptContextScene();
    if (scene)
        scene->sendRemoteEvent(entity, StringHash(eventType), eventData, receiver);
}

static void SendRemoteEntityEventDelayTTL(Entity* entity, const std::string& eventType, const VariantMap& eventData, Connection* receiver, float delay, unsigned short timeToLive)
{
    Scene* scene = getScriptContextScene();
    if (scene)
        scene->sendRemoteEvent(entity, StringHash(eventType), eventData, receiver, delay, timeToLive);
}

static void ConstructNetUpdateInfo(NetUpdateInfo* ptr)
{
    ptr->mConnection = 0;
    ptr->mFrameNumber = 0;
    ptr->mFrameAck = 0;
    ptr->mRemoteFrameNumber = 0;
    ptr->mRoundTripTime = 0.0f;
}

static Connection* NetUpdateInfoGetConnection(NetUpdateInfo* ptr)
{
    return ptr->mConnection;
}

static void registerConnection(asIScriptEngine* engine)
{
    engine->RegisterEnum("JoinState");
    engine->RegisterEnumValue("JoinState", "JS_NOTINSCENE", JS_NOTINSCENE);
    engine->RegisterEnumValue("JoinState", "JS_PREPARESCENE", JS_PREPARESCENE);
    engine->RegisterEnumValue("JoinState", "JS_SENDFULLUPDATE", JS_SENDFULLUPDATE);
    engine->RegisterEnumValue("JoinState", "JS_WAITFORACK", JS_WAITFORACK);
    engine->RegisterEnumValue("JoinState", "JS_SENDDELTAS", JS_SENDDELTAS);
    
    engine->RegisterObjectType("Connection", 0, asOBJ_REF);
    engine->RegisterObjectBehaviour("Connection", asBEHAVE_ADDREF, "void f()", asMETHOD(Connection, addRef), asCALL_THISCALL);
    engine->RegisterObjectBehaviour("Connection", asBEHAVE_RELEASE, "void f()", asMETHOD(Connection, releaseRef), asCALL_THISCALL);
    engine->RegisterObjectMethod("Connection", "void disconnect()", asMETHOD(Connection, disconnect), asCALL_THISCALL);
    engine->RegisterObjectMethod("Connection", "void forceDisconnect()", asMETHOD(Connection, forceDisconnect), asCALL_THISCALL);
    engine->RegisterObjectMethod("Connection", "void setUserName(const string& in)", asMETHOD(Connection, setUserName), asCALL_THISCALL);
    engine->RegisterObjectMethod("Connection", "void setControls(const Controls& in)", asMETHOD(Connection, setControls), asCALL_THISCALL);
    engine->RegisterObjectMethod("Connection", "Scene@+ getScene() const", asMETHOD(Connection, getScene), asCALL_THISCALL);
    engine->RegisterObjectMethod("Connection", "const string& getUserName() const", asMETHOD(Connection, getUserName), asCALL_THISCALL);
    engine->RegisterObjectMethod("Connection", "bool isConnected() const", asMETHOD(Connection, isConnected), asCALL_THISCALL);
    engine->RegisterObjectMethod("Connection", "JoinState getJoinState() const", asMETHOD(Connection, getJoinState), asCALL_THISCALL);
    engine->RegisterObjectMethod("Connection", "uint16 getFrameNumber() const", asMETHOD(Connection, getFrameNumber), asCALL_THISCALL);
    engine->RegisterObjectMethod("Connection", "uint16 getFrameAck() const", asMETHOD(Connection, getFrameAck), asCALL_THISCALL);
    engine->RegisterObjectMethod("Connection", "float getRoundTripTime() const", asMETHOD(Connection, getRoundTripTime), asCALL_THISCALL);
    engine->RegisterObjectMethod("Connection", "const Controls& getControls() const", asMETHOD(Connection, getControls), asCALL_THISCALL);
    engine->RegisterObjectMethod("Connection", "const Vector3& getPosition() const", asMETHOD(Connection, getPosition), asCALL_THISCALL);
    
    // Register Variant getPtr() for Connection
    engine->RegisterObjectMethod("Connection", "Connection@+ getConnection() const", asFUNCTION(getVariantPtr<Connection>), asCALL_CDECL_OBJLAST);
    
    engine->RegisterObjectType("NetUpdateInfo", sizeof(NetUpdateInfo), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_C);
    engine->RegisterObjectBehaviour("NetUpdateInfo", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructNetUpdateInfo), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("NetUpdateInfo", "Connection@+ get_connection()", asFUNCTION(NetUpdateInfoGetConnection), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectProperty("NetUpdateInfo", "uint16 frameNumber", offsetof(NetUpdateInfo, mFrameNumber));
    engine->RegisterObjectProperty("NetUpdateInfo", "uint16 frameAck", offsetof(NetUpdateInfo, mFrameAck));
    engine->RegisterObjectProperty("NetUpdateInfo", "uint16 remoteFrameNumber", offsetof(NetUpdateInfo, mRemoteFrameNumber));
    engine->RegisterObjectProperty("NetUpdateInfo", "float roundTripTime", offsetof(NetUpdateInfo, mRoundTripTime));
    
    // Add missing Scene & Entity methods that have to do with Connection
    engine->RegisterObjectMethod("Scene", "void resetOwner(Connection@+)", asMETHOD(Scene, resetOwner), asCALL_THISCALL);
    engine->RegisterObjectMethod("Entity", "bool checkSync(Connection@+) const", asMETHOD(Entity, checkSync), asCALL_THISCALL);
    engine->RegisterObjectMethod("Entity", "bool checkPrediction(Connection@+) const", asMETHOD(Entity, checkPrediction), asCALL_THISCALL);
    
    // Add remote event functionality as global functions because the Scene can be found out through the context
    engine->RegisterGlobalFunction("void sendRemoteEvent(const string& in, const VariantMap& in)", asFUNCTION(SendRemoteEventToAll), asCALL_CDECL);
    engine->RegisterGlobalFunction("void sendRemoteEvent(const string& in, const VariantMap& in, Connection@+)", asFUNCTION(SendRemoteEvent), asCALL_CDECL);
    engine->RegisterGlobalFunction("void sendRemoteEvent(const string& in, const VariantMap& in, Connection@+, float, uint16)", asFUNCTION(SendRemoteEventDelayTTL), asCALL_CDECL);
    engine->RegisterGlobalFunction("void sendRemoteEvent(Entity@+, const string& in, const VariantMap& in)", asFUNCTION(SendRemoteEntityEventToAll), asCALL_CDECL);
    engine->RegisterGlobalFunction("void sendRemoteEvent(Entity@+, const string& in, const VariantMap& in, Connection@+)", asFUNCTION(SendRemoteEntityEvent), asCALL_CDECL);
    engine->RegisterGlobalFunction("void sendRemoteEvent(Entity@+, const string& in, const VariantMap& in, Connection@+, float, uint16)", asFUNCTION(SendRemoteEntityEventDelayTTL), asCALL_CDECL);
    engine->RegisterGlobalFunction("Connection@+ getRemoteEventSender()", asFUNCTION(getRemoteEventSender), asCALL_CDECL);
}

static void ConstructSceneInfo(SceneInfo* ptr)
{
    new(ptr) SceneInfo();
}

static void ConstructSceneInfoCopy(const SceneInfo& info, SceneInfo* ptr)
{
    new(ptr) SceneInfo(info);
}

static void DestructSceneInfo(SceneInfo* ptr)
{
    ptr->~SceneInfo();
}

static bool ClientConnectAddress(const std::string& address, unsigned short port, Client* ptr)
{
    return ptr->connect(address, port);
}

static bool ClientConnectAddressUserName(const std::string& address, unsigned short port, const std::string& userName, Client* ptr)
{
    return ptr->connect(address, port, userName);
}

static bool IsClient()
{
    return getClient() != 0;
}

static void registerClient(asIScriptEngine* engine)
{
    engine->RegisterObjectType("SceneInfo", sizeof(SceneInfo), asOBJ_VALUE | asOBJ_APP_CLASS_CDA);
    engine->RegisterObjectBehaviour("SceneInfo", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructSceneInfo), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectBehaviour("SceneInfo", asBEHAVE_CONSTRUCT, "void f(const SceneInfo& in)", asFUNCTION(ConstructSceneInfoCopy), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectBehaviour("SceneInfo", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructSceneInfo), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("SceneInfo", "SceneInfo &opAssign(const SceneInfo& in)", asMETHOD(SceneInfo, operator =), asCALL_THISCALL);
    engine->RegisterObjectProperty("SceneInfo", "string name", offsetof(SceneInfo, mName));
    engine->RegisterObjectProperty("SceneInfo", "uint numUsers", offsetof(SceneInfo, mNumUsers));
    engine->RegisterObjectProperty("SceneInfo", "int netFps", offsetof(SceneInfo, mNetFps));
    engine->RegisterObjectProperty("SceneInfo", "string fileName", offsetof(SceneInfo, mFileName));
    
    engine->RegisterObjectType("Client", 0, asOBJ_REF);
    engine->RegisterObjectBehaviour("Client", asBEHAVE_ADDREF, "void f()", asMETHOD(Client, addRef), asCALL_THISCALL);
    engine->RegisterObjectBehaviour("Client", asBEHAVE_RELEASE, "void f()", asMETHOD(Client, releaseRef), asCALL_THISCALL);
    engine->RegisterObjectMethod("Client", "void setScene(Scene@+)", asMETHOD(Client, setScene), asCALL_THISCALL);
    engine->RegisterObjectMethod("Client", "void setMaxPredictionTime(float)", asMETHOD(Client, setMaxPredictionTime), asCALL_THISCALL);
    engine->RegisterObjectMethod("Client", "bool connect(const string& in, uint16)", asFUNCTION(ClientConnectAddress), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("Client", "bool connect(const string& in, uint16, const string& in)", asFUNCTION(ClientConnectAddressUserName), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("Client", "bool connect(const string& in, uint16, const string& in, const VariantMap& in)", asMETHOD(Client, connect), asCALL_THISCALL);
    engine->RegisterObjectMethod("Client", "void disconnect()", asMETHOD(Client, disconnect), asCALL_THISCALL);
    engine->RegisterObjectMethod("Client", "void setControls(const Controls& in)", asMETHOD(Client, setControls), asCALL_THISCALL);
    engine->RegisterObjectMethod("Client", "void setPosition(const Vector3& in)", asMETHOD(Client, setPosition), asCALL_THISCALL);
    engine->RegisterObjectMethod("Client", "Scene@+ getScene() const", asMETHOD(Client, getScene), asCALL_THISCALL);
    engine->RegisterObjectMethod("Client", "Connection@+ getServerConnection() const", asMETHOD(Client, getServerConnection), asCALL_THISCALL);
    engine->RegisterObjectMethod("Client", "bool isConnected() const", asMETHOD(Client, isConnected), asCALL_THISCALL);
    engine->RegisterObjectMethod("Client", "bool isJoinPending() const", asMETHOD(Client, isJoinPending), asCALL_THISCALL);
    engine->RegisterObjectMethod("Client", "bool isJoined() const", asMETHOD(Client, isJoined), asCALL_THISCALL);
    engine->RegisterObjectMethod("Client", "int getNetFps() const", asMETHOD(Client, getNetFps), asCALL_THISCALL);
    engine->RegisterObjectMethod("Client", "float getMaxPredictionTime() const", asMETHOD(Client, getMaxPredictionTime), asCALL_THISCALL);
    engine->RegisterObjectMethod("Client", "const Controls& getControls() const", asMETHOD(Client, getControls), asCALL_THISCALL);
    engine->RegisterObjectMethod("Client", "const Vector3& getPosition() const", asMETHOD(Client, getPosition), asCALL_THISCALL);
    engine->RegisterObjectMethod("Client", "uint getNumFileTransfers() const", asMETHOD(Client, getNumFileTransfers), asCALL_THISCALL);
    engine->RegisterObjectMethod("Client", "const string& getDownloadDirectory() const", asMETHOD(Client, getDownloadDirectory), asCALL_THISCALL);
    engine->RegisterObjectMethod("Client", "string getFileTransferStatus() const", asMETHOD(Client, getFileTransferStatus), asCALL_THISCALL);
    engine->RegisterObjectMethod("Client", "const SceneInfo& getSceneInfo() const", asMETHOD(Client, getSceneInfo), asCALL_THISCALL);
    
    engine->RegisterGlobalFunction("bool isClient()", asFUNCTION(IsClient), asCALL_CDECL);
    engine->RegisterGlobalFunction("Client@+ getClient()", asFUNCTION(getClient), asCALL_CDECL);
    engine->RegisterGlobalFunction("Client@+ get_client()", asFUNCTION(getClient), asCALL_CDECL);
}

static CScriptArray* ServerGetScenes(Server* ptr)
{
    const std::vector<SharedPtr<Scene> >& scenes = ptr->getScenes();
    std::vector<Scene*> result;
    for (unsigned i = 0; i < scenes.size(); ++i)
        result.push_back(scenes[i]);
    return vectorToHandleArray<Scene*>(result, "array<Scene@>");
}

static CScriptArray* ServerGetConnections(Server* ptr)
{
    const std::vector<SharedPtr<Connection> >& connections = ptr->getConnections();
    std::vector<Connection*> result;
    for (unsigned i = 0; i < connections.size(); ++i)
        result.push_back(connections[i]);
    return vectorToHandleArray<Connection*>(result, "array<Connection@>");
}

static bool IsServer()
{
    return getServer() != 0;
}

static void registerServer(asIScriptEngine* engine)
{
    engine->RegisterObjectType("Server", 0, asOBJ_REF);
    engine->RegisterObjectBehaviour("Server", asBEHAVE_ADDREF, "void f()", asMETHOD(Server, addRef), asCALL_THISCALL);
    engine->RegisterObjectBehaviour("Server", asBEHAVE_RELEASE, "void f()", asMETHOD(Server, releaseRef), asCALL_THISCALL);
    engine->RegisterObjectMethod("Server", "void setNetFps(int)", asMETHOD(Server, setNetFps), asCALL_THISCALL);
    engine->RegisterObjectMethod("Server", "void setMaxSceneRevisions(int)", asMETHOD(Server, setMaxSceneRevisions), asCALL_THISCALL);
    engine->RegisterObjectMethod("Server", "void setStayRelevantTime(int)", asMETHOD(Server, setStayRelevantTime), asCALL_THISCALL);
    engine->RegisterObjectMethod("Server", "void addScene(Scene@+)", asMETHOD(Server, addScene), asCALL_THISCALL);
    engine->RegisterObjectMethod("Server", "void removeScene(Scene@+)", asMETHOD(Server, removeScene), asCALL_THISCALL);
    engine->RegisterObjectMethod("Server", "bool start(uint16)", asMETHOD(Server, start), asCALL_THISCALL);
    engine->RegisterObjectMethod("Server", "void stop()", asMETHOD(Server, stop), asCALL_THISCALL);
    engine->RegisterObjectMethod("Server", "bool setClientScene(Connection@+, Scene@+)", asMETHOD(Server, setClientScene), asCALL_THISCALL);
    engine->RegisterObjectMethod("Server", "int getNetFps() const", asMETHOD(Server, getNetFps), asCALL_THISCALL);
    engine->RegisterObjectMethod("Server", "int getMaxSceneRevisions() const", asMETHOD(Server, getMaxSceneRevisions), asCALL_THISCALL);
    engine->RegisterObjectMethod("Server", "int getStayRelevantTime() const", asMETHOD(Server, getStayRelevantTime), asCALL_THISCALL);
    engine->RegisterObjectMethod("Server", "bool isRunning() const", asMETHOD(Server, isRunning), asCALL_THISCALL);
    engine->RegisterObjectMethod("Server", "bool hasScene(Scene@+) const", asMETHOD(Server, hasScene), asCALL_THISCALL);
    engine->RegisterObjectMethod("Server", "array<Scene@>@ getScenes() const", asFUNCTION(ServerGetScenes), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("Server", "array<Connection@>@ getConnections() const", asFUNCTION(ServerGetConnections), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("Server", "uint getNumUsersInScene(Scene@+) const", asMETHOD(Server, getNumUsersInScene), asCALL_THISCALL);
    engine->RegisterGlobalFunction("bool isServer()", asFUNCTION(IsServer), asCALL_CDECL);
    engine->RegisterGlobalFunction("Server@+ getServer()", asFUNCTION(getServer), asCALL_CDECL);
    engine->RegisterGlobalFunction("Server@+ get_server()", asFUNCTION(getServer), asCALL_CDECL);
}

static void ParticleEmitterLoadParameters(XMLFile* file, ParticleEmitter* ptr)
{
    try
    {
        ptr->loadParameters(file, getResourceCache());
    }
    catch (Exception& e)
    {
        SAFE_RETHROW(e);
    }
}

static void registerParticleEmitter(asIScriptEngine* engine)
{
    registerGeometryNode<ParticleEmitter>(engine, "ParticleEmitter");
    engine->RegisterObjectMethod("ParticleEmitter", "void setMaterial(Material@+)", asMETHODPR(ParticleEmitter, setMaterial, (Material*), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("ParticleEmitter", "void setBillboardsRelative(bool)", asMETHODPR(ParticleEmitter, setBillboardsRelative, (bool), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("ParticleEmitter", "void setBillboardsSorted(bool)", asMETHODPR(ParticleEmitter, setBillboardsSorted, (bool), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("ParticleEmitter", "void setScaleBillboards(bool)", asMETHODPR(ParticleEmitter, setScaleBillboards, (bool), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("ParticleEmitter", "void setAnimationLodBias(float)", asMETHODPR(ParticleEmitter, setAnimationLodBias, (float), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("ParticleEmitter", "Material@+ getMaterial() const", asMETHODPR(ParticleEmitter, getMaterial, () const, Material*), asCALL_THISCALL);
    engine->RegisterObjectMethod("ParticleEmitter", "bool getBillboardsRelative() const", asMETHODPR(ParticleEmitter, getBillboardsRelative, () const, bool), asCALL_THISCALL);
    engine->RegisterObjectMethod("ParticleEmitter", "bool getBillboardsSorted() const", asMETHODPR(ParticleEmitter, getBillboardsSorted, () const, bool), asCALL_THISCALL);
    engine->RegisterObjectMethod("ParticleEmitter", "bool getScaleBillboards() const", asMETHODPR(ParticleEmitter, getScaleBillboards, () const, bool), asCALL_THISCALL);
    engine->RegisterObjectMethod("ParticleEmitter", "float getAnimationLodBias() const", asMETHODPR(ParticleEmitter, getAnimationLodBias, () const, float), asCALL_THISCALL);
    engine->RegisterObjectMethod("ParticleEmitter", "void loadParameters(XMLFile@+ file)", asFUNCTION(ParticleEmitterLoadParameters), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("ParticleEmitter", "void setActive(bool, bool)", asMETHOD(ParticleEmitter, setActive), asCALL_THISCALL);
    engine->RegisterObjectMethod("ParticleEmitter", "XMLFile@+ getParameterSource() const", asMETHOD(ParticleEmitter, getParameterSource), asCALL_THISCALL);
    engine->RegisterObjectMethod("ParticleEmitter", "bool isActive() const", asMETHOD(ParticleEmitter, isActive), asCALL_THISCALL);
    engine->RegisterObjectMethod("ParticleEmitter", "uint getNumParticles() const", asMETHOD(ParticleEmitter, getNumParticles), asCALL_THISCALL);
    registerRefCasts<Component, ParticleEmitter>(engine, "Component", "ParticleEmitter");
    registerRefCasts<Node, ParticleEmitter>(engine, "Node", "ParticleEmitter");
}

static void registerDebugHud(asIScriptEngine* engine)
{
    engine->RegisterGlobalProperty("const uint DEBUGHUD_SHOW_NONE", (void*)&DEBUGHUD_SHOW_NONE);
    engine->RegisterGlobalProperty("const uint DEBUGHUD_SHOW_STATS", (void*)&DEBUGHUD_SHOW_STATS);
    engine->RegisterGlobalProperty("const uint DEBUGHUD_SHOW_MODE", (void*)&DEBUGHUD_SHOW_MODE);
    engine->RegisterGlobalProperty("const uint DEBUGHUD_SHOW_PROFILER", (void*)&DEBUGHUD_SHOW_PROFILER);
    
    engine->RegisterObjectType("DebugHud", 0, asOBJ_REF);
    engine->RegisterObjectBehaviour("DebugHud", asBEHAVE_ADDREF, "void f()", asMETHOD(DebugHud, addRef), asCALL_THISCALL);
    engine->RegisterObjectBehaviour("DebugHud", asBEHAVE_RELEASE, "void f()", asMETHOD(DebugHud, releaseRef), asCALL_THISCALL);
    engine->RegisterObjectMethod("DebugHud", "void setFont(Font@+, int)", asMETHOD(DebugHud, setFont), asCALL_THISCALL);
    engine->RegisterObjectMethod("DebugHud", "void setMode(uint)", asMETHOD(DebugHud, setMode), asCALL_THISCALL);
    engine->RegisterObjectMethod("DebugHud", "void setProfilerInterval(float)", asMETHOD(DebugHud, setProfilerInterval), asCALL_THISCALL);
    engine->RegisterObjectMethod("DebugHud", "void toggle(uint)", asMETHOD(DebugHud, toggle), asCALL_THISCALL);
    engine->RegisterObjectMethod("DebugHud", "void toggleAll()", asMETHOD(DebugHud, toggleAll), asCALL_THISCALL);
    engine->RegisterObjectMethod("DebugHud", "uint getMode() const", asMETHOD(DebugHud, getMode), asCALL_THISCALL);
    engine->RegisterObjectMethod("DebugHud", "float getProfilerInterval() const", asMETHOD(DebugHud, getProfilerInterval), asCALL_THISCALL);
    
    engine->RegisterGlobalFunction("DebugHud@+ getDebugHud()", asFUNCTION(getDebugHud), asCALL_CDECL);
    engine->RegisterGlobalFunction("DebugHud@+ get_debugHud()", asFUNCTION(getDebugHud), asCALL_CDECL);
}

static Scene* EngineCreateScene(const std::string& name, const BoundingBox& octreeSize, unsigned octreeLevels, bool usePhysics, Engine* ptr)
{
    SharedPtr<Scene> newScene = ptr->createScene(name, octreeSize, octreeLevels, usePhysics);
    // The shared pointer will go out of scope, so have to increment the reference count
    // (here an auto handle can not be used)
    newScene->addRef();
    return newScene.getPtr();
}

static void registerEngine(asIScriptEngine* engine)
{
    engine->RegisterGlobalProperty("const int DEBUGDRAW_NONE", (void*)&DEBUGDRAW_NONE);
    engine->RegisterGlobalProperty("const int DEBUGDRAW_RENDERING", (void*)&DEBUGDRAW_RENDERING);
    engine->RegisterGlobalProperty("const int DEBUGDRAW_PHYSICS", (void*)&DEBUGDRAW_PHYSICS);
    
    engine->RegisterObjectType("Engine", 0, asOBJ_REF);
    engine->RegisterObjectBehaviour("Engine", asBEHAVE_ADDREF, "void f()", asMETHOD(Engine, addRef), asCALL_THISCALL);
    engine->RegisterObjectBehaviour("Engine", asBEHAVE_RELEASE, "void f()", asMETHOD(Engine, releaseRef), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "Scene@ createScene(const string& in, const BoundingBox& in, uint, bool)", asFUNCTION(EngineCreateScene), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("Engine", "Client@ createClient(const string& in)", asMETHOD(Engine, createClient), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "Server@ createServer()", asMETHOD(Engine, createServer), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "DebugHud@ createDebugHud()", asMETHOD(Engine, createDebugHud), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "void removeClient()", asMETHOD(Engine, removeClient), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "void removeServer()", asMETHOD(Engine, removeServer), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "void setMinFps(int)", asMETHOD(Engine, setMinFps), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "void setMaxFps(int)", asMETHOD(Engine, setMaxFps), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "void setMaxInactiveFps(int)", asMETHOD(Engine, setMaxInactiveFps), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "void setDebugDrawMode(int)", asMETHOD(Engine, setDebugDrawMode), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "void setFlushGPU(bool)", asMETHOD(Engine, setFlushGPU), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "void exit()", asMETHOD(Engine, exit), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "void dumpProfilingData()", asMETHOD(Engine, dumpProfilingData), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "void dumpResources()", asMETHOD(Engine, dumpResources), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "int getMinFps() const", asMETHOD(Engine, getMinFps), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "int getMaxFps() const", asMETHOD(Engine, getMaxFps), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "int getMaxInactiveFps() const", asMETHOD(Engine, getMaxInactiveFps), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "int getDebugDrawMode() const", asMETHOD(Engine, getDebugDrawMode), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "bool getFlushGPU() const", asMETHOD(Engine, getFlushGPU), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "bool isInitialized() const", asMETHOD(Engine, isInitialized), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "bool isExiting() const", asMETHOD(Engine, isExiting), asCALL_THISCALL);
    engine->RegisterObjectMethod("Engine", "bool isHeadless() const", asMETHOD(Engine, isHeadless), asCALL_THISCALL);
    
    engine->RegisterGlobalFunction("Engine@+ getEngine()", asFUNCTION(getEngine), asCALL_CDECL);
    engine->RegisterGlobalFunction("Engine@+ get_engine()", asFUNCTION(getEngine), asCALL_CDECL);
}

void registerEngineLibrary(asIScriptEngine* engine)
{
    registerConnection(engine);
    registerClient(engine);
    registerServer(engine);
    registerParticleEmitter(engine);
    registerDebugHud(engine);
    registerEngine(engine);
}
