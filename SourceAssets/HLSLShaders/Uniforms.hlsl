// Vertex shader parameters
uniform float3 cAmbientStartColor : register(C0);
uniform float3 cAmbientEndColor : register(C1);
uniform float3 cCameraPos : register(C2);
uniform float3x3 cCameraRot : register(C3);
uniform float4 cDepthMode : register(C6);
uniform float3 cFrustumSize : register(C7);
uniform float4 cGBufferOffsets : register(C8);
uniform float3 cLightDir : register(C9);
uniform float4 cLightPos : register(C10);
uniform float4x3 cModel : register(C11);
uniform float4x4 cViewProj : register(C14);
uniform float4 cUOffset : register(C18);
uniform float4 cVOffset : register(C19);
uniform float3 cViewRightVector : register(C20);
uniform float3 cViewUpVector : register(C21);
uniform float4x3 cZone : register(C22);
uniform float4x4 cLightMatrices[4] : register(C25);
uniform float4x3 cSkinMatrices[64] : register(C41);
uniform float4 cVertexLights[6*3] : register(C233);

// Pixel shader parameters
uniform float3 cAmbientColor : register(C0);
uniform float2 cDepthReconstruct : register(C1);
uniform float4 cFogParams : register(C2);
uniform float3 cFogColor : register(C3);
uniform float4 cLightColor : register(C4);
uniform float4 cLightPosPS : register(C5);
uniform float3 cLightDirPS : register(C6);
uniform float4 cMatDiffColor : register(C7);
uniform float3 cMatEmissiveColor : register(C8);
uniform float2 cMatSpecProperties : register(C9);
uniform float2 cSampleOffsets : register(C10);
uniform float4 cShadowCubeAdjust : register(C11);
uniform float4 cShadowDepthFade : register(C12);
uniform float4 cShadowIntensity : register(C13);
uniform float4 cShadowSplits : register(C14);
uniform float4x4 cLightMatricesPS[4] :  register(C15);
