#include "Uniforms.vert"
#include "Transform.vert"

varying vec2 vTexCoord;
#ifdef VERTEXCOLOR
    varying vec4 vColor;
#endif
varying vec4 vLightVec;
#ifdef SPECULAR
    varying vec3 vEyeVec;
#endif
#ifndef NORMALMAP
    varying vec3 vNormal;
#endif
#ifdef SHADOW
    #if defined(DIRLIGHT)
        varying vec4 vShadowPos[4];
    #elif defined(SPOTLIGHT)
        varying vec4 vShadowPos;
    #else
        varying vec3 vShadowPos;
    #endif
#endif
#ifdef SPOTLIGHT
    varying vec4 vSpotPos;
#endif
#ifdef POINTLIGHT
    varying vec3 vCubeMaskVec;
#endif

void main()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vTexCoord = GetTexCoord(iTexCoord);

    #ifdef VERTEXCOLOR
        vColor = iColor;
    #endif

    #ifdef NORMALMAP
        vec3 vNormal;
        vec3 vTangent;
        vec3 vBitangent;
    #endif

    vNormal = GetWorldNormal(modelMatrix);
    vec4 projWorldPos = vec4(worldPos, 1.0);

    #ifdef SHADOW
        // Shadow projection: transform from world space to shadow space
        #if defined(DIRLIGHT)
            vShadowPos[0] = cLightMatrices[0] * projWorldPos;
            vShadowPos[1] = cLightMatrices[1] * projWorldPos;
            vShadowPos[2] = cLightMatrices[2] * projWorldPos;
            vShadowPos[3] = cLightMatrices[3] * projWorldPos;
        #elif defined(SPOTLIGHT)
            vShadowPos = cLightMatrices[1] * projWorldPos;
        #else
            vShadowPos = worldPos - cLightPos.xyz;
        #endif
    #endif

    #ifdef SPOTLIGHT
        // Spotlight projection: transform from world space to projector texture coordinates
        vSpotPos = cLightMatrices[0] * projWorldPos;
    #endif

    #ifdef POINTLIGHT
        vCubeMaskVec = mat3(cLightMatrices[0][0].xyz, cLightMatrices[0][1].xyz, cLightMatrices[0][2].xyz) * (cLightPos.xyz - worldPos);
    #endif

    #ifdef NORMALMAP
        vTangent = GetWorldTangent(modelMatrix);
        vBitangent = cross(vTangent, vNormal) * iTangent.w;
        mat3 tbn = mat3(vTangent, vBitangent, vNormal);
        #ifdef DIRLIGHT
            vLightVec = vec4(cLightDir * tbn, GetDepth(gl_Position));
        #else
            vLightVec = vec4((cLightPos.xyz - worldPos) * tbn * cLightPos.w, GetDepth(gl_Position));
        #endif
        #ifdef SPECULAR
            vEyeVec = (cCameraPos - worldPos) * tbn;
        #endif
    #else
        #ifdef DIRLIGHT
            vLightVec = vec4(cLightDir, GetDepth(gl_Position));
        #else
            vLightVec = vec4((cLightPos.xyz - worldPos) * cLightPos.w, GetDepth(gl_Position));
        #endif
        #ifdef SPECULAR
            vEyeVec = cCameraPos - worldPos;
        #endif
    #endif
}
