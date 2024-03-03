#version 460
#define SKY_AMBIENT_MULTIPLIER 0.1
#define cloudy  0.5 //0.0 clear sky
#define haze  0.01 * (cloudy*20.)

//Environment
const float R0 = 6360e3; //planet radius //6360e3 actual 6371km
const float Ra = 6380e3; //atmosphere radius //6380e3 troposphere 8 to 14.5km
const float I = 10.; //sun light power, 10.0 is normal
const float SI = 5.; //sun intensity for sun
const float g = 0.45; //light concentration .76 //.45 //.6  .45 is normaL
const float g2 = g * g;

const float s = 0.999; //light concentration for sun
const float s2 = s;
const float Hr = 8e3; //Rayleigh scattering top //8e3
const float Hm = 1.2e3; //Mie scattering top //1.3e3
vec3 bM = vec3(21e-6); //normal mie // vec3(21e-6)
//Rayleigh scattering (sky color, atmospheric up to 8km)
vec3 bR = vec3(5.8e-6, 13.5e-6, 33.1e-6); //normal earth

layout(set = 0, binding = 0) uniform  SceneData{   
	vec4 ambientColor;
	vec4 sunlightDirection; //w for sun power
	vec4 sunlightColor;
	vec4 viewPos;
    vec4 waterData; //a.time, b.WaterTurbulence c.WaterAbsorption d.color
    vec4 cloudData; //cloud absortion, 
} sceneData;

layout(set = 1, binding = 1) uniform sampler3D densityTex;
layout(set = 1, binding = 2) uniform sampler3D lightingTex;
layout(set = 1, binding = 3) uniform samplerCube skyBox;

layout (location = 0) in vec3 inPos;

layout (location = 0) out vec4 outFragColor;

vec3 GetSunLightDirection()
{
    return normalize(sceneData.sunlightDirection.xyz);
}

vec3 GetSunLightColor()
{
    return sceneData.sunlightColor.w * sceneData.sunlightColor.xyz;
    // return 0.9 * vec3(0.9, 0.5, 0.4);
}

float BeerLambert1(float absorptionCoefficient, float distanceTraveled) {
    return exp(-absorptionCoefficient * distanceTraveled);
}

vec3 GetUVW(vec3 posModel) {
    return (posModel + 1.0) * 0.5;
}

vec3 GetAmbientShadowColor()
{
    return vec3(0, 0, 0.2);
}

vec3 GetAmbientSkyColor()
{
    return SKY_AMBIENT_MULTIPLIER * mix(vec3(0.2, 0.5, 0.8),vec3(0.7, 0.75, 0.9), max(1, 0.0));
}

vec4 CloudColor(vec3 dir)
{
    int numSteps = 64;
    float stepSize = 0.8 / float(numSteps);
    vec3 volumeAlbedo = vec3(1, 1, 1);
    vec4 color = vec4(0, 0, 0, 1);
    vec3 marchPosition = vec3(0.0f);
    float cloudAbsorption = sceneData.cloudData.x;//fix
    float contrastFactor = 0.8;
    for (int i =0; i < numSteps; ++i)
    {
        vec3 uvw = GetUVW(marchPosition);
        float density = texture(densityTex, uvw).r;
        float lighting = texture(lightingTex, uvw).r;
        // density = pow(density, contrastFactor);
        if (density > 1e-3){
            float prevAlpha = color.a;
            color.a *= BeerLambert1(cloudAbsorption * density, stepSize);
            float absorptionFromMarch = prevAlpha - color.a;
            color.rgb += absorptionFromMarch * volumeAlbedo * (1.5*GetSunLightColor()+GetAmbientShadowColor()) * density * lighting;
                        //  * HenyeyGreensteinPhase(PushConstants.lightDir.xyz, dirModel, PushConstants.aniso);
        }

        marchPosition += dir*stepSize;
        if (abs(marchPosition.x) > 1 || abs(marchPosition.y) > 1 || abs(marchPosition.z) > 1)
            break;

        if (color.a < 1e-3)
            break;
    }
    color = clamp(color, 0.0, 1.0);
    color.a = 1.0 - color.a;
    return color + vec4(GetAmbientSkyColor(),0);
    // if (inModelPos.z > -0.7) color.a = 0; 
    // return vec4(volumeAlbedo * (mix(GetAmbientShadowColor(), 1.5*GetSunLightColor(), vec3(color.a)) + GetAmbientSkyColor()), min(color.a, 1.0f));
}
vec3 C = vec3(0., -R0, 0.); //planet center

void densities(in vec3 pos, out float rayleigh, out float mie) {
	float h = length(pos - C) - R0;
	rayleigh =  exp(-h/Hr);
	mie = exp(-h/Hm) + haze; 
}

float escape(in vec3 p, in vec3 d, in float R) {
	vec3 v = p - C;
	float b = dot(v, d);
	float c = dot(v, v) - R*R;
	float det2 = b * b - c;
	if (det2 < 0.) return -1.;
	float det = sqrt(det2);
	float t1 = -b - det, t2 = -b + det;
	return (t1 >= 0.) ? t1 : t2;
}

vec3 scatter(vec3 o, vec3 d) {
    vec3 color = vec3(0);
	float L = escape(o, d, Ra);	
	float mu = dot(d, GetSunLightDirection());
	float opmu2 = 1. + mu*mu;
	float phaseR = .0596831 * opmu2;
	float phaseM = .1193662 * (1. - g2) * opmu2 / ((2. + g2) * pow(1. + g2 - 2.*g*mu, 1.5));
    float phaseS = .1193662 * (1. - s2) * opmu2 / ((2. + s2) * pow(1. + s2 - 2.*s*mu, 1.5));
	
	float depthR = 0., depthM = 0.;
	vec3 R = vec3(0.), M = vec3(0.);
    float step = 16;
	float dl = L / float(step);
	for (int i = 0; i < step; ++i) {
		float l = float(i) * dl;
		vec3 p = (o + d * l);

		float dR, dM;
		densities(p, dR, dM);
		dR *= dl; dM *= dl;
		depthR += dR;
		depthM += dM;

		float Ls = escape(p, GetSunLightDirection(), Ra);
		if (Ls > 0.) {
			float dls = Ls / float(step);
			float depthRs = 0., depthMs = 0.;
			for (int j = 0; j < step; ++j) {
				float ls = float(j) * dls;
				vec3 ps = ( p + GetSunLightDirection() * ls );
				float dRs, dMs;
				densities(ps, dRs, dMs);
				depthRs += dRs * dls;
				depthMs += dMs * dls;
			}

			vec3 A = exp(-(bR * (depthRs + depthR) + bM * (depthMs + depthM)));
			R += (A * dR);
			M += A * dM ;
		}
	}

    color = (I) *(M * bM * (phaseM )); // Mie scattering
    color += (SI) *(M * bM *phaseS); //Sun
    color += (I) *(R * bR * phaseR); //Rayleigh scattering
    return color + 0.1 *(bM*depthM);
}

vec3 GetBaseSkyColor(vec3 rayDirection)
{
    return scatter(vec3(0,sceneData.viewPos.y,0), rayDirection);
}

vec3 GetSkyColor(in vec3 rayDirection)
{
    vec3 skyColor = scatter(vec3(0,sceneData.viewPos.y,0), rayDirection);
    vec4 cloudColor = CloudColor(rayDirection );
    skyColor = mix(skyColor, cloudColor.rgb, cloudColor.a);

    return skyColor.xyz;
}

void main() 
{
	if (inPos.y < -0.05) discard;
	outFragColor = vec4(GetSkyColor(normalize(inPos)), 1);
}