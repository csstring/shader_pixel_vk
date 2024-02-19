#version 460

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"

layout( push_constant ) uniform constants
{
	mat4 render_matrix;
	mat4 view;
	mat4 proj;
} PushConstants;


void main() 
{
	gl_Position = PushConstants.proj * PushConstants.view * PushConstants.render_matrix * vec4(inPos, 1.0f);
}
