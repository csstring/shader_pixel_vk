#version 460

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;

#extension GL_GOOGLE_include_directive : require

#include "input_structures.glsl"


layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;

layout( push_constant ) uniform constants
{
	mat4 render_matrix;
} PushConstants;

void main() 
{
	outNormal = inNormal;
	outColor = inColor;
	outUV = inUV;
	gl_Position =  sceneData.viewproj * PushConstants.render_matrix * vec4(inPos, 1.0f);

	vec4 pos = sceneData.view * vec4(inPos, 1.0);

	outNormal = mat3(sceneData.view) * inNormal;
	vec3 lPos = mat3(sceneData.view) * sceneData.sunlightDirection.xyz;
	outLightVec = sceneData.sunlightDirection.xyz - pos.xyz;
	outViewVec = sceneData.viewPos.xyz - pos.xyz;	

}
