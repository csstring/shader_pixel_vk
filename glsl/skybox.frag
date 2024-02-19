#version 450
layout(set = 1, binding = 3) uniform samplerCube skyBox;

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = texture(skyBox, inUVW);
}