#version 460

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;

struct ObjectData{
	mat4 model;
	vec4 color;
};

layout(std140, set = 1, binding = 0) readonly buffer ObjectBuffer {
	ObjectData objects[];
} objectBuffer;

layout(set = 0, binding = 0) uniform  CameraBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
	vec4 ambientColor;
	vec4 sunlightDirection;
	vec4 sunlightColor;
} cameraData;

layout(push_constant) uniform Params {
    vec4 lightPos;
    vec4 camPos;
} params;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;

void main() 
{
	outNormal = inNormal;
	outColor = inColor;
	outUV = inUV;
	gl_Position = cameraData.viewproj * objectBuffer.objects[gl_BaseInstance].model * vec4(inPos, 1.0f);
	
	vec4 pos = cameraData.view * vec4(inPos, 1.0);
	outNormal = mat3(cameraData.view) * inNormal;
	vec3 lPos = mat3(cameraData.view) * params.lightPos.xyz;
	outLightVec = params.lightPos.xyz - pos.xyz;
	outViewVec = params.camPos.xyz - pos.xyz;	
}