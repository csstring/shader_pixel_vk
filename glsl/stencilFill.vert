#version 460

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;

layout(set = 0, binding = 0) uniform  SceneData{   
	vec4 ambientColor;
	vec4 sunlightDirection; //w for sun power
	vec4 sunlightColor;
	vec4 viewPos;
} sceneData;

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
