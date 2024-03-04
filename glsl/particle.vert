#version 460

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;

struct Particle {
    vec4 position;//wlife
    vec4 velocity;
};

layout(std140, set = 1, binding = 0) readonly buffer ParticleSSBOIn {
   Particle particles[];
};

layout( push_constant ) uniform constants
{
	mat4 render_matrix;
	mat4 view;
	mat4 proj;
	mat4 normal;
} PushConstants;

void main() 
{
	vec3 scaleUpPos = (PushConstants.render_matrix * vec4(inPos, 1)).xyz + particles[gl_InstanceIndex].position.xyz;
	gl_Position =  PushConstants.proj * PushConstants.view * vec4(scaleUpPos,1);
}
