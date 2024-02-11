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

layout (location = 0) out vec3 outPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec3 outViewVec;
layout (location = 3) out vec3 outLightVec;
layout (location = 4) out mat4 invModel;

layout( push_constant ) uniform constants
{
	mat4 render_matrix;
	mat4 view;
	mat4 proj;
} PushConstants;

void main() 
{
  gl_Position = PushConstants.proj * PushConstants.view * PushConstants.render_matrix * vec4(inPos.xyz, 1.0);

	outPos = vec3(PushConstants.render_matrix * vec4(inPos, 1.0));
	outNormal = mat3(PushConstants.render_matrix) * inNormal;

	outLightVec = sceneData.sunlightDirection.xyz - outPos.xyz;
	outViewVec = -outPos.xyz;
  invModel = inverse(PushConstants.render_matrix);
}
