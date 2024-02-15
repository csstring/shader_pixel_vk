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
layout (location = 3) out vec3 outpos;
layout (location = 4) out vec3 outviewPos;

layout( push_constant ) uniform constants
{
	mat4 render_matrix;
	mat4 view;
	mat4 proj;
} PushConstants;

void main() 
{
	outNormal = inNormal;
	outColor = inColor;
	outUV = inUV;
	outpos = inPos;
	outviewPos = (PushConstants.view * vec4(inPos,1)).xyz;
	gl_Position =  PushConstants.proj * PushConstants.view * PushConstants.render_matrix * vec4(inPos, 1.0f);
}
