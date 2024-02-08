#version 460

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;

layout(set = 0, binding = 0) uniform  SceneData{   

	mat4 view;
	mat4 proj;
	mat4 viewproj;
	vec4 ambientColor;
	vec4 sunlightDirection; //w for sun power
	vec4 sunlightColor;
	vec4 viewPos;
} sceneData;


layout (location = 0) out vec3 outUVW;

layout( push_constant ) uniform constants
{
	mat4 render_matrix;
} PushConstants;

void main() 
{
  outUVW = inPos;
  outUVW.xy *= -1.0;
  mat4 viewMat = mat4(mat3(PushConstants.render_matrix));
	gl_Position = sceneData.viewproj * viewMat * vec4(inPos, 1.0f);
}
