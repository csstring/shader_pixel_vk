#version 460

struct ObjectData{
	mat4 model;
	vec4 color;
};

layout(std140, set = 1, binding = 0) readonly buffer ObjectBuffer {
	ObjectData objects[];
} objectBuffer;

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 uv;
layout (location = 3) in vec2 vColor;

layout (location = 0) out vec3 modelPos;
layout (location = 1) out mat4 worldInv;


layout(set = 0, binding = 0) uniform  CameraBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
	vec4 ambientColor;
	vec4 sunlightDirection;
	vec4 sunlightColor;
	vec4 viewPos;
} cameraData;


void main()
{
	gl_Position = cameraData.viewproj * objectBuffer.objects[gl_BaseInstance].model *vec4(vPosition, 1.0f);
	modelPos = vPosition;
	worldInv = inverse(objectBuffer.objects[gl_BaseInstance].model);
}
