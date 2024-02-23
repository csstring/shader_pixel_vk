#version 460
layout(set = 0, binding = 0) uniform  SceneData{   
	vec4 ambientColor;
	vec4 sunlightDirection; //w for sun power
	vec4 sunlightColor;
	vec4 viewPos;
    vec4 waterData; //a.time, b.WaterTurbulence c.WaterAbsorption d.color
    vec4 cloudData; //cloud absortion, aniso
} sceneData;


layout(set = 1, binding = 1) uniform sampler3D densityTex;
layout(set = 1, binding = 2) uniform sampler3D lightingTex;
layout(set = 1, binding = 3) uniform samplerCube skyBox;

layout( push_constant ) uniform constants
{
	mat4 render_matrix;
	mat4 view;
	mat4 proj;
} PushConstants;

vec3 GetUVW(vec3 posModel) {
    return (posModel + 1.0) * 0.5;
}

float BeerLambert(float absorptionCoefficient, float distanceTraveled) {
    return exp(-absorptionCoefficient * distanceTraveled);
}

float HenyeyGreensteinPhase(vec3 L, vec3 V, float aniso) {
    float cosTheta = dot(L, -V);
    float g = aniso;
    return (1.0 - g * g) / (4.0 * 3.141592 * pow(abs(1.0 + g * g - 2.0 * g * cosTheta), 1.5));
}
float sdSphere( vec3 p, float s )
{
    p.y = 0;
  return length(p)-s;
}
float sdTorus( vec3 p, vec2 t )
{
  vec2 q = vec2(length(p.xz)-t.x,p.y);
  return length(q)-t.y;
}

float sdEllipsoid( vec3 p, vec3 r )
{
  float k0 = length(p/r);
  float k1 = length(p/(r*r));
  return k0*(k0-1.0)/k1;
}

layout(location = 0) in vec3 inPosModel;
layout(location = 1) in mat4 worldInv; 

layout(location = 0) out vec4 outColor;

void main() {

    vec3 eyeModel = (worldInv * vec4(sceneData.viewPos)).xyz;
    vec3 dirModel = normalize(inPosModel - eyeModel);

    int numSteps = 64;
    float stepSize = 2.0 / float(numSteps);

    vec3 volumeAlbedo = vec3(1, 1, 1);

    vec4 color = vec4(0, 0, 0, 1);
    vec3 posModel = inPosModel + dirModel * 1e-6;

    for (int i = 0; i < numSteps; ++i) {
        vec3 uvw = GetUVW(posModel);
        float density = texture(densityTex, uvw).r;
        float lighting = texture(lightingTex, uvw).r; 

        // float sdf = sdSphere(posModel, 0.43);

        // if (sdf > 0.0f){
        //     density *= clamp(1.0-sdf*9.0f, 0.0,1.0f);
        // }
        if (density > 1e-3) {

            float prevAlpha = color.a;
            color.a *= BeerLambert(sceneData.cloudData.x * density, stepSize);
            float absorptionFromMarch = prevAlpha - color.a;
            color.rgb += absorptionFromMarch * volumeAlbedo * (sceneData.sunlightColor.xyz * sceneData.sunlightColor.w)
                         * density * lighting
                         * HenyeyGreensteinPhase(normalize(sceneData.sunlightDirection.xyz), dirModel, sceneData.cloudData.y);
        }

        posModel += dirModel * stepSize;

        if (abs(posModel.x) > 1 || abs(posModel.y) > 1 || abs(posModel.z) > 1)
            break;

        if (color.a < 1e-3)
            break;
    }

    color = clamp(color, 0.0, 1.0);
    color.a = 1.0 - color.a;
    outColor = color;
    // outColor = vec4(1.0f);
}