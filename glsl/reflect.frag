#version 450

layout(set = 1, binding = 1) uniform samplerCube samplerCubeMap;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;

layout(set = 0, binding = 0) uniform  SceneData{   

	mat4 view;
	mat4 proj;
	mat4 viewproj;
	vec4 ambientColor;
	vec4 sunlightDirection; //w for sun power
	vec4 sunlightColor;
	vec4 viewPos;
} sceneData;

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inViewVec;
layout (location = 3) in vec3 inLightVec;
layout (location = 4) in mat4 invModel;

layout (location = 0) out vec4 outFragColor;

void main()
{
	vec3 cI = normalize (inPos);
	vec3 cR = reflect (cI, normalize(inNormal));

	cR = vec3(invModel * vec4(cR, 0.0));
	// Convert cubemap coordinates into Vulkan coordinate space
	cR.xy *= -1.0;

	vec4 color = texture(samplerCubeMap, cR, 0);

	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightVec);
	vec3 V = normalize(inViewVec);
	vec3 R = reflect(-L, N);
	vec3 ambient = vec3(0.5) * color.rgb;
	vec3 diffuse = max(dot(N, L), 0.0) * vec3(1.0);
	vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * vec3(0.5);
	outFragColor = vec4(ambient + diffuse * color.rgb + specular, 1.0);
}
