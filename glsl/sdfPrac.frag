#version 460
#define PERFORMANCE_MODE 0
#define ULTRA_MODE 1
// #define MAX_VOLUME_ENTRIES 1
#if PERFORMANCE_MODE
#define ALLOW_KEYBOARD_INPUT 0
#define SECONDARY_REFLECTION 0
#define ADD_WHITE_WATER 0
#define MAX_SDF_SPHERE_STEPS 12
#define SDF_START_STEP_SIZE 3.0
#define SDF_END_STEP_SIZE 100.0
#define MAX_VOLUME_MARCH_STEPS 10
#define BINARY_SEARCH_STEPS 5
#define MAX_OPAQUE_SHADOW_MARCH_STEPS 4
#define SHADOW_FACTOR_STEP_SIZE 10.0
#else
#if ULTRA_MODE
#define MAX_VOLUME_ENTRIES 2
#else
#define MAX_VOLUME_ENTRIES 1
#endif

#define ALLOW_KEYBOARD_INPUT 1
#define SECONDARY_REFLECTION 1
#define ADD_WHITE_WATER 1
#define MAX_SDF_SPHERE_STEPS 20
#define SDF_START_STEP_SIZE 1.5
#define SDF_END_STEP_SIZE 100.0
#define MAX_VOLUME_MARCH_STEPS 20
#define BINARY_SEARCH_STEPS 6
#define MAX_OPAQUE_SHADOW_MARCH_STEPS 10
#define SHADOW_FACTOR_STEP_SIZE 7.0
#endif

#define GROUND_LEVEL 0.0
#define WATER_LEVEL 20.0
#define PI 3.14
#define LARGE_NUMBER 1e20
#define EPSILON 0.0001


#define NUM_SPHERES 10

#define SAND_FLOOR_OBJECT_ID 0
#define CORAL_ROCK_BASE_OBJECT_ID 1
#define NUM_OBJECTS (CORAL_ROCK_BASE_OBJECT_ID + NUM_SPHERES)

#define INVALID_OBJECT_ID int(-1)

#define AIR_IOR 1.0

#define SKY_AMBIENT_MULTIPLIER 0.1

#define MAX_WATER_DISPLACEMENT 15.0
#define MIN_REFLECTION_COEFFECIENT 0.0

#define SCENE_TYPE_OCEAN 1
#define SCENE_TYPE_SIMPLIFIED_OCEAN 2
#define SCENE_TYPE_OPAQUE 3
float WaterIor;
float WaterTurbulence;
float WaterAbsorption;
vec3 WaterColor;
// #extension GL_GOOGLE_include_directive : require
// #include "input_structures.glsl"
layout(set = 0, binding = 0) uniform  SceneData{   
	vec4 ambientColor;
	vec4 sunlightDirection; //w for sun power
	vec4 sunlightColor;
	vec4 viewPos;
    vec4 waterData; //a.time, b.WaterTurbulence c.WaterAbsorption d.color
    vec4 cloudData; //cloud absortion, 
} sceneData;

layout(set = 1, binding = 0) uniform GLTFMaterialData{   

	vec4 colorFactors;
	vec4 metal_rough_factors;
	
} materialData;

layout( push_constant ) uniform constants
{
	mat4 render_matrix;
	mat4 view;
	mat4 proj;
} PushConstants;

layout(set = 1, binding = 1) uniform sampler3D densityTex;
layout(set = 1, binding = 2) uniform sampler3D lightingTex;
layout(set = 1, binding = 3) uniform samplerCube skyBox;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inModelPos;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inPos;
layout (location = 4) in mat4 invModel;

layout (location = 0) out vec4 outFragColor;

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
//vec3 bR = vec3(5.8e-6, 33.1e-6, 13.5e-6); //purple
//vec3 bR = vec3( 63.5e-6, 13.1e-6, 50.8e-6 ); //green
//vec3 bR = vec3( 13.5e-6, 23.1e-6, 115.8e-6 ); //yellow
//vec3 bR = vec3( 5.5e-6, 15.1e-6, 355.8e-6 ); //yeellow
//vec3 bR = vec3(3.5e-6, 333.1e-6, 235.8e-6 ); //red-purple

vec3 GetSunLightDirection()
{
    return normalize(sceneData.sunlightDirection.xyz);
}

vec3 GetSunLightColor()
{
    return sceneData.sunlightColor.w * sceneData.sunlightColor.xyz;
    // return 0.9 * vec3(0.9, 0.5, 0.4);
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

// this can be explained: http://www.scratchapixel.com/lessons/3d-advanced-lessons/simulating-the-colors-of-the-sky/atmospheric-scattering/
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
    float step = 8;
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


// --------------------------------------------//
//               Noise Functions
// --------------------------------------------//
// Taken from Inigo Quilez's Rainforest ShaderToy:
// https://www.shadertoy.com/view/4ttSWf
float hash1( float n )
{
    return fract( n*17.0*fract( n*0.3183099 ) );
}

// Taken from Inigo Quilez's Rainforest ShaderToy:
// https://www.shadertoy.com/view/4ttSWf
float noise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 w = fract(x);
    
    vec3 u = w*w*w*(w*(w*6.0-15.0)+10.0);
    
    float n = p.x + 317.0*p.y + 157.0*p.z;
    
    float a = hash1(n+0.0);
    float b = hash1(n+1.0);
    float c = hash1(n+317.0);
    float d = hash1(n+318.0);
    float e = hash1(n+157.0);
	float f = hash1(n+158.0);
    float g = hash1(n+474.0);
    float h = hash1(n+475.0);

    float k0 =   a;
    float k1 =   b - a;
    float k2 =   c - a;
    float k3 =   e - a;
    float k4 =   a - b - c + d;
    float k5 =   a - c - e + g;
    float k6 =   a - b - e + f;
    float k7 = - a + b + c - d + e - f - g + h;

    return -1.0+2.0*(k0 + k1*u.x + k2*u.y + k3*u.z + k4*u.x*u.y + k5*u.y*u.z + k6*u.z*u.x + k7*u.x*u.y*u.z);
}

const mat3 m3  = mat3( 0.00,  0.80,  0.60,
                      -0.80,  0.36, -0.48,
                      -0.60, -0.48,  0.64 );

// Taken from Inigo Quilez's Rainforest ShaderToy:
// https://www.shadertoy.com/view/4ttSWf
float fbm( in vec3 x, int iterations )
{
    float f = 2.0;
    float s = 0.5;
    float a = 0.0;
    float b = 0.5;
    for( int i= 0; i<iterations; i++ )
    {
        float n = noise(x);
        a += b*n;
        b *= s;
        x = f*m3*x;
    }
	return a;
}

// Taken from Inigo Quilez's Rainforest ShaderToy:
// https://www.shadertoy.com/view/4ttSWf
float fbm_4( in vec3 x )
{
    return fbm(x, 4);
}

// Taken from https://iquilezles.org/articles/distfunctions
float sdPlane( vec3 p )
{
	return p.y;
}

// Taken from https://iquilezles.org/articles/distfunctions
float sdSmoothUnion( float d1, float d2, float k ) 
{
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h); 
}

// Taken from https://iquilezles.org/articles/distfunctions
float sdSmoothSubtraction( float d1, float d2, float k ) {
    float h = clamp( 0.5 - 0.5*(d2+d1)/k, 0.0, 1.0 );
    return mix( d2, -d1, h ) + k*h*(1.0-h); 
}

vec3 sdTranslate(vec3 pos, vec3 translate)
{
    return pos -= translate;
}

// Taken from https://iquilezles.org/articles/distfunctions
float sdSphere( vec3 p, vec3 origin, float s )
{
  p = sdTranslate(p, origin);
  return length(p)-s;
}


struct Sphere
{
    vec3 origin;
    float radius;
};
    //+-40 40 60~100
void GetSphere(int index, out vec3 origin, out float radius)
{
    Sphere spheres[NUM_SPHERES];
    spheres[0] = Sphere(vec3(28, GROUND_LEVEL, 70), 12.0);
    spheres[1] = Sphere(vec3(13, GROUND_LEVEL , 75), 13.5);
    spheres[2] = Sphere(vec3(-25, GROUND_LEVEL , 80), 14.5);
    spheres[3] = Sphere(vec3(-24, GROUND_LEVEL, 85), 15.0);
    spheres[4] = Sphere(vec3(15, GROUND_LEVEL, 90), 13.0);



    origin = spheres[index].origin;
    radius = spheres[index].radius;

}

float GetWaterWavesDisplacement(vec3 position, float time)
{
    return sceneData.waterData.w * sin(position.x / 15.0 + time * 1.3) + sceneData.waterData.w * cos(position.z / 150.0 + time / 1.1) +4;
}

float GetWaterNoise(vec3 position, float time)
{
    
    return WaterTurbulence * fbm_4(position / 15.0 + time / 3.0);
}

float QueryOceanDistanceField( in vec3 pos, float time)
{    
    return pos.y - (GetWaterWavesDisplacement(pos, time) + GetWaterNoise(pos, time) + WATER_LEVEL);
}

float QueryVolumetricDistanceField( in vec3 pos, float time)
{   
    float minDist = QueryOceanDistanceField(pos, time);
    minDist = sdSmoothSubtraction(sdSphere(pos, sceneData.viewPos.xyz, 35.0) +  5.0 * fbm_4(pos / vec3(12, 20, 12)  - time / 5.0), minDist, 12.0);   
    minDist = sdSmoothUnion(minDist, sdPlane(pos - vec3(0, GROUND_LEVEL - 1.0, 0)), 13.0);

    return minDist;
}

float IntersectVolumetric(in vec3 rayOrigin, in vec3 rayDirection, float maxT, float time, int sceneType, out bool intersectFound)
{
    float t = 0.0f;
    float sdfValue = 0.0;
    float stepSize = SDF_START_STEP_SIZE;
    float stepIncrement = float(SDF_END_STEP_SIZE - SDF_START_STEP_SIZE) / float(MAX_SDF_SPHERE_STEPS);
    for(int i=0; i<MAX_SDF_SPHERE_STEPS; i++ )
    {
	    sdfValue = sceneType == SCENE_TYPE_OCEAN ?
            QueryVolumetricDistanceField(rayOrigin+rayDirection*t, time) :
        	QueryOceanDistanceField(rayOrigin+rayDirection*t, time);
        stepSize += stepIncrement;
        
        if( sdfValue < 0.0 || t>maxT ) break;
        t += max(sdfValue, stepSize);
    }
    if(sdfValue < 0.0f)
    {
        float start = 0.0;
        float end = stepSize;
        t -= stepSize;
        
        for(int j = 0; j < BINARY_SEARCH_STEPS; j++)
        {
            float midPoint = (start + end) * 0.5;
            vec3 nextMarchPosition = rayOrigin + (t + midPoint) * rayDirection;
            float sdfValue = (sceneType == SCENE_TYPE_OCEAN) ?
                QueryVolumetricDistanceField(nextMarchPosition, time) :
                QueryOceanDistanceField(nextMarchPosition, time);
            
            // Use the SDF to nudget the mid point closer to the actual edge
            midPoint = clamp(midPoint + sdfValue, start, end);
            if(sdfValue < 0.0)
            {
                end = midPoint;
            }
            else
            {
                start = midPoint;
            }
        }
        t += end;
    }
    
    intersectFound = t<maxT && sdfValue < 0.0f;
    return t;
}

// Taken from https://iquilezles.org/articles/normalsSDF
vec3 GetVolumeNormal( in vec3 pos, float time, int sceneType )
{
    vec3 n = vec3(0.0);
    for( int i= 0; i<4; i++ )
    {
        vec3 e = 0.5773*(2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
        n += e*
            ((sceneType == SCENE_TYPE_OCEAN) ?
                QueryVolumetricDistanceField(pos+0.5*e, time) :
                QueryOceanDistanceField(pos+0.5*e, time));
    }
    return normalize(n);
}

struct Material
{
    vec3 albedo;
    float shininess;
};

Material GetMaterial(int objectID)
{
    if(objectID == SAND_FLOOR_OBJECT_ID)
    {
        return Material(0.9 * vec3(1.0, 1.0, 0.8), 50.0);
    }
    else // if(objectID >= CORAL_ROCK_BASE_OBJECT_ID) // it's coral
    {
        return Material(vec3(0.3, 0.4, 0.2), 3.0);
    }
}

// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection
float PlaneIntersection(vec3 rayOrigin, vec3 rayDirection, vec3 planeOrigin, vec3 planeNormal) 
{ 
    float t = -1.0f;
    float denom = dot(-planeNormal, rayDirection); 
    if (denom > EPSILON) { 
        vec3 rayToPlane = planeOrigin - rayOrigin; 
        return dot(rayToPlane, -planeNormal) / denom; 
    } 
 
    return t; 
} 
    
float SphereIntersection(
    in vec3 rayOrigin, 
    in vec3 rayDirection, 
    in vec3 sphereCenter, 
    in float sphereRadius)
{
      vec3 eMinusC = rayOrigin - sphereCenter;
      float dDotD = dot(rayDirection, rayDirection);

      float discriminant = dot(rayDirection, (eMinusC)) * dot(rayDirection, (eMinusC))
         - dDotD * (dot(eMinusC, eMinusC) - sphereRadius * sphereRadius);

      if (discriminant < 0.0) 
         return -1.0;

      float firstIntersect = (dot(-rayDirection, eMinusC) - sqrt(discriminant))
             / dDotD;
      
      float t = firstIntersect;
      return t;
}


void UpdateIfIntersected(
    inout float t,
    in float intersectionT, 
    in int intersectionObjectID,
    out int objectID)
{    
    if(intersectionT > EPSILON && intersectionT < t)
    {
        objectID = intersectionObjectID;
        t = intersectionT;
    }
}

float SandHeightMap(vec3 position)
{
    float sandGrainNoise = 0.1 * fbm(position * 10.0, 2);
    float sandDuneDisplacement = 0.7 * sin(10.0 * fbm_4(10.0 + position / 40.0));
	return sandGrainNoise  + sandDuneDisplacement;
}

float QueryOpaqueDistanceField(vec3 position, int objectID)
{
    if(objectID == SAND_FLOOR_OBJECT_ID)
    {
        return sdPlane(position) + SandHeightMap(position);
    }
    else
    {
        vec3 origin;
        float radius;
        GetSphere(objectID - CORAL_ROCK_BASE_OBJECT_ID, origin, radius);
        return sdSphere(position, origin, radius) + fbm_4(position);
    }
}

// Taken from https://iquilezles.org/articles/normalsSDF
vec3 GetOpaqueNormal( in vec3 pos, int objectID )
{
    vec3 n = vec3(0.0);
    for( int i= 0; i<4; i++ )
    {
        vec3 e = 0.5773*(2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
        n += e*QueryOpaqueDistanceField(pos+0.5*e, objectID);
    }
    return normalize(n);
}

float IntersectOpaqueScene(in vec3 rayOrigin, in vec3 rayDirection, out int objectID)
{
    float intersectionT = LARGE_NUMBER;

    float t = LARGE_NUMBER;
    objectID = INVALID_OBJECT_ID;

    for(int i =  0; i < NUM_SPHERES; i++)
    {
        vec3 origin;
        float radius;
        GetSphere(i, origin, radius);
            UpdateIfIntersected(
            t,
            SphereIntersection(rayOrigin, rayDirection, origin, radius),
            CORAL_ROCK_BASE_OBJECT_ID + i,
            objectID);
    }
    
    UpdateIfIntersected(
        t,
        PlaneIntersection(rayOrigin, rayDirection, vec3(0, GROUND_LEVEL, 0), vec3(0, 1, 0)),
        SAND_FLOOR_OBJECT_ID,
        objectID);
    if (t > 100) //최대 시야 거리
        t = LARGE_NUMBER;
//  충돌 체크는 되는데 계속 마지막 물 히트에 걸리네?

    return t;
}

float Specular(in vec3 reflection, in vec3 lightDirection, float shininess)
{
    return 0.05 * pow(max(0.0, dot(reflection, lightDirection)), shininess);
}

vec3 Diffuse(in vec3 normal, in vec3 lightVec, in vec3 diffuse)
{
    float nDotL = dot(normal, lightVec);
    return clamp(nDotL * diffuse, 0.0, 1.0);
}

vec3 BeerLambert(vec3 absorption, float dist)
{
    return exp(-absorption * dist);
}

vec3 GetShadowFactor(in vec3 rayOrigin, in vec3 rayDirection, in int maxSteps, in float minMarchSize)
{
    float t = 0.0f;
    vec3 shadowFactor = vec3(1.0f);
    float signedDistance = 0.0;
    bool enteredVolume = false;
    for(int i =  0; i < maxSteps; i++)
    {         
        float marchSize = max(minMarchSize, abs(signedDistance));
        t += marchSize;

        vec3 position = rayOrigin + t*rayDirection;

        signedDistance = QueryVolumetricDistanceField(position, sceneData.waterData.x);
        if(signedDistance < 0.0)
        {
            // Soften the shadows towards the edges to simulate an area light
            float softEdgeMultiplier = min(abs(signedDistance / 5.0), 1.0);
            shadowFactor *= BeerLambert(WaterAbsorption * softEdgeMultiplier / WaterColor, marchSize);
            enteredVolume = true;
        }
        else if(enteredVolume)
        {
            // Optimization taking advantage of the shape of the water. The water isn't
            // concave therefore if we've entered it once and are exiting, we're done
            break;
        }
    }
    return shadowFactor;
}

float GetApproximateIntersect(vec3 position, vec3 rayDirection)
{
    float distanceToPlane;
    
    // Special case for rays parallel to the ground plane to avoid divide
    // by zero issues
    if(abs(rayDirection.y) < 0.01)
    {
        distanceToPlane = LARGE_NUMBER;
    }
    else if(position.y < GROUND_LEVEL || position.y > WATER_LEVEL)
    {
        distanceToPlane = 0.0f;
    }
    else if(rayDirection.y > 0.0)
    {
		distanceToPlane = (WATER_LEVEL - position.y) / rayDirection.y;
        vec3 intersectPosition = position + distanceToPlane * rayDirection;
        
        distanceToPlane = max(0.0, distanceToPlane);
    }
    else
    {
        distanceToPlane = (position.y - GROUND_LEVEL) / abs(rayDirection.y);
    }
    return distanceToPlane;
}

vec3 GetApproximateShadowFactor(vec3 position, vec3 rayDirection)
{
    float distanceToPlane = GetApproximateIntersect(position, rayDirection);
	return BeerLambert(WaterAbsorption / WaterColor, distanceToPlane);
}

float seed = 0.;
float rand() { return fract(sin(seed++ + sceneData.waterData.x)*43758.5453123); }

float smoothVoronoi( in vec2 x )
{
    ivec2 p = ivec2(floor( x ));
    vec2  f = fract( x );

    float res = 0.0;
    for( int j=-1; j<=1; j++ )
    for( int i=-1; i<=1; i++ )
    {
        ivec2 b = ivec2( i, j );
        vec2  r = vec2( b ) - f + noise( vec3(p + b, 0.0) );
        float d = length( r );

        res += exp( -32.0*d );
    }
    return -(1.0/32.0)*log( res );
}

vec3 GetBaseSkyColor(vec3 rayDirection)
{
    return scatter(vec3(0,sceneData.viewPos.y,0), rayDirection);
}

vec3 GetAmbientSkyColor()
{
    return SKY_AMBIENT_MULTIPLIER * mix(vec3(0.2, 0.5, 0.8),vec3(0.7, 0.75, 0.9), max(1, 0.0));
}

vec3 GetAmbientShadowColor()
{
    return vec3(0, 0, 0.2);
}

float GetCloudDenity(vec3 position)
{
    float time = sceneData.waterData.x * 0.25;
    vec3 noisePosition = position + vec3(0.0, 0.0, time);
    float noise = fbm_4(noisePosition);
    float noiseCutoff = -0.3;
    return max(0.0, 3.0f * (noise - noiseCutoff));
}

float HenyeyGreensteinPhase(vec3 L, vec3 V, float aniso) {
    float cosTheta = dot(L, -V);
    float g = aniso;
    return (1.0 - g * g) / (4.0 * 3.141592 * pow(abs(1.0 + g * g - 2.0 * g * cosTheta), 1.5));
}

float BeerLambert1(float absorptionCoefficient, float distanceTraveled) {
    return exp(-absorptionCoefficient * distanceTraveled);
}

vec3 GetUVW(vec3 posModel) {
    return (posModel + 1.0) * 0.5;
}

vec4 CloudColor(vec3 dir)
{
    int numSteps = 32;
    float stepSize = 0.7 / float(numSteps);
    vec3 volumeAlbedo = vec3(1, 1, 1);
    vec4 color = vec4(0, 0, 0, 1);
    vec3 marchPosition = vec3(0.0f);
    float cloudAbsorption = 8;//fix
    float contrastFactor = 0.4;
    for (int i =0; i < numSteps; ++i)
    {
        vec3 uvw = GetUVW(marchPosition);
        float density = texture(densityTex, uvw).r;
        float lighting = texture(lightingTex, uvw).r;
        density = pow(density, contrastFactor);
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

vec4 GetCloudColor(vec3 position)
{
    float cloudDensity = GetCloudDenity(position);
    vec3 cloudAlbedo = vec3(1, 1, 1);
    float cloudAbsorption = 0.6;
    float marchSize = 0.25;

    vec3 lightFactor = vec3(1, 1, 1);
    {
        vec3 marchPosition = position;
        int selfShadowSteps = 4;
        for(int i = 0; i < selfShadowSteps; i++)
        {
            marchPosition += GetSunLightDirection() * marchSize;
            float density = cloudAbsorption * GetCloudDenity(marchPosition);
            lightFactor *= BeerLambert(vec3(density, density, density), marchSize);
        }
    }

    return vec4(
        cloudAlbedo * 
        	(mix(GetAmbientShadowColor(), 1.3 * GetSunLightColor(), lightFactor) +
             GetAmbientSkyColor()), 
        min(cloudDensity, 1.0));
}

vec3 GetSkyColor(in vec3 rayDirection)
{
    // vec3 skyColor = GetBaseSkyColor(rayDirection);
    // vec4 cloudColor = CloudColor(rayDirection );
    // skyColor = mix(skyColor, cloudColor.rgb, cloudColor.a);

    // return skyColor.xyz;
    if (rayDirection.y < 0.02) rayDirection.y = 0.02;
    return texture(skyBox, (rayDirection)).xyz;
}

float FresnelFactor(
    float CurrentIOR,
    float NewIOR,
    vec3 Normal,
    vec3 RayDirection)
{
    float ReflectionCoefficient = 
        ((CurrentIOR - NewIOR) / (CurrentIOR + NewIOR)) *
        ((CurrentIOR - NewIOR) / (CurrentIOR + NewIOR));
    return 
        clamp(ReflectionCoefficient + (1.0 - ReflectionCoefficient) * pow(1.0 - dot(Normal, -RayDirection), 5.0), MIN_REFLECTION_COEFFECIENT, 1.0); 
}

vec3 SandParallaxOcclusionMapping(vec3 position, vec3 view)
{
    int pomCount = 6;
    float marchSize = 0.3;
    for(int i = 0; i < pomCount; i++)
    {
        if(position.y < GROUND_LEVEL -  SandHeightMap(position)) break;
        position += view * marchSize;
    }
    return position;
}

void CalculateLighting(vec3 position, vec3 view, int objectID, inout vec3 color, bool useFastLighting)
{   
    Material material = GetMaterial(objectID);
    float sdfValue = QueryVolumetricDistanceField(position, sceneData.waterData.x);
    bool bUnderWater = sdfValue < 0.0;

    float wetnessFactor = 0.0;
    if(objectID == SAND_FLOOR_OBJECT_ID && !useFastLighting)
    {
        float wetSandDistance = 0.7;
		if(sdfValue <= wetSandDistance)
        {
            // Darken the sand albedo to make it look wet
            float fadeEdge = 0.2;
            wetnessFactor = 1.0 - max(0.0, (sdfValue - (wetSandDistance - fadeEdge)) / fadeEdge);
			material.albedo *= material.albedo * mix(1.0, 0.5, wetnessFactor);
        }
        
        position = SandParallaxOcclusionMapping(position, view);
    }

    vec3 normal = GetOpaqueNormal(position, objectID);
    vec3 reflectionDirection = reflect(view, normal);
   
    int shadowObjectID = INVALID_OBJECT_ID;
    if(!useFastLighting)
    {
        IntersectOpaqueScene(position, GetSunLightDirection(), shadowObjectID);
    }
    
	vec3 shadowFactor = vec3(0.0, 0.0, 0.0);
    // vec3 shadowFactor = vec3(0.5);
    if(shadowObjectID == INVALID_OBJECT_ID)
    {
        float t;
        shadowFactor = useFastLighting ? 
                GetApproximateShadowFactor(position + normal*1e-2, GetSunLightDirection()) :
                GetShadowFactor(position + normal*1e-2, GetSunLightDirection(), MAX_OPAQUE_SHADOW_MARCH_STEPS, SHADOW_FACTOR_STEP_SIZE);
        
        // Small back splash of the sky ambient color to fake a bit of gi
        color += shadowFactor * material.albedo * mix(0.4 * GetAmbientShadowColor(), GetSunLightColor(), max(0.0, dot(normal, GetSunLightDirection())));
        // color += shadowFactor * material.albedo * mix(0.4 * GetAmbientShadowColor(), GetSunLightColor(), 0.5);
        color += shadowFactor * GetSunLightColor() * Specular(reflectionDirection, GetSunLightDirection(), material.shininess);
 	    
        if(!useFastLighting)
        {
            // Fake caustics
            float waterNoise = fract(GetWaterNoise(position, sceneData.waterData.x));
            float causticMultiplier = bUnderWater ? 7.0 : (1.0 - shadowFactor.r);
            color += material.albedo * causticMultiplier * 0.027 *  pow(
                smoothVoronoi(position.xz / 4.0 + 
                          vec2(sceneData.waterData.x, sceneData.waterData.x + 3.0) + 
                          3.0 * vec2(cos(waterNoise), sin(waterNoise))), 5.0);
        }
        
    }
    
    // Add a bit of reflection to wet sand to make it look like 
    // there's some water left over
    if(!useFastLighting && wetnessFactor > 0.0)
    {
        // Water fills in the holes in the sand and generally
        // makes the surface planar so we can assume the normal is
        // pointing straight up
        vec3 wetNormal = vec3(0, 1, 0);
        vec3 reflectionDirection = reflect(view, wetNormal);
        float fresnel = FresnelFactor(AIR_IOR, WaterIor, wetNormal, view);
        color += shadowFactor * wetnessFactor * fresnel * GetSkyColor(reflectionDirection);
    } 
    color += GetAmbientSkyColor() * material.albedo;
}
//Opaque 애들은 반사를 하면 안되는데 자꾸 하늘 색 가져오네 
vec3 Render( in vec3 rayOrigin, in vec3 rayDirection)
{
    vec3 accumulatedColor = vec3(0.0f);
    vec3 accumulatedColorMultiplier = vec3(1.0);
    int materialID = INVALID_OBJECT_ID;
    float t = IntersectOpaqueScene(rayOrigin, rayDirection, materialID);
    vec3 opaquePosition = rayOrigin + t*rayDirection;
    
    bool outsideVolume = true;
    for(int entry = 0; entry < MAX_VOLUME_ENTRIES; entry++) 
    { 
        if(!outsideVolume) break;
        
        bool firstEntry = (entry == 0);
        bool intersectFound = false;
        //물 시작거리 IntersectOpaqueScene에서 구한 최대 거리 이상으로 나가면 intersectFound == false
        float volumeStart = 
            IntersectVolumetric(
                rayOrigin,
                rayDirection, 
                t, 
                sceneData.waterData.x,
                (firstEntry ? SCENE_TYPE_OCEAN : SCENE_TYPE_SIMPLIFIED_OCEAN),
                intersectFound);
        // if (!intersectFound && t == LARGE_NUMBER) discard;
        if(!intersectFound && entry == 0 && t == LARGE_NUMBER) return GetSkyColor(rayDirection);
        if (!intersectFound) break;
		else
        {
            outsideVolume = false;
            rayOrigin = rayOrigin + rayDirection * volumeStart;// primary ray 도착지점q
            vec3 volumeNormal = GetVolumeNormal(rayOrigin, sceneData.waterData.x, SCENE_TYPE_OCEAN);
            vec3 reflection = reflect( rayDirection, volumeNormal);
            float fresnelFactor = FresnelFactor(AIR_IOR, WaterIor, volumeNormal, rayDirection);
            float waterShininess = 100.0;

            float whiteWaterFactor = 0.0;
            float whiteWaterMaxHeight = 5.0;
            float groundBlendFactor = min(1.0, (rayOrigin.y - GROUND_LEVEL) * 0.75);
            #if ADD_WHITE_WATER
            if(firstEntry && rayOrigin.y <= whiteWaterMaxHeight)
            {
                WaterIor = mix(1.0, WaterIor, groundBlendFactor);
                
                vec3 voronoisePosition = rayOrigin / 1.5 + vec3(0, -sceneData.waterData.x * 2.0, sin(sceneData.waterData.x));
            	float noiseValue = abs(fbm(voronoisePosition, 2));
            	voronoisePosition += 1.0 * vec3(cos(noiseValue), 0.0, sin(noiseValue));
                
                float heightLerp =  (whiteWaterMaxHeight - rayOrigin.y) / whiteWaterMaxHeight;
                whiteWaterFactor = abs(smoothVoronoi(voronoisePosition.xz)) * heightLerp;
                whiteWaterFactor = clamp(whiteWaterFactor, 0.0, 1.0);
                whiteWaterFactor = pow(whiteWaterFactor, 0.2) * heightLerp;
                whiteWaterFactor *= mix(abs(fbm(rayOrigin + vec3(0, -sceneData.waterData.x * 5.0, 0), 2)), 1.0, heightLerp);
                whiteWaterFactor *= groundBlendFactor;
                
                vec3 shadowFactor =  GetShadowFactor(rayOrigin + volumeNormal * 1e-2, GetSunLightDirection(), MAX_OPAQUE_SHADOW_MARCH_STEPS, SHADOW_FACTOR_STEP_SIZE);
                vec3 diffuse = 0.5 * shadowFactor * GetSunLightColor() + 
                    0.7 * shadowFactor * mix(GetAmbientShadowColor(), GetSunLightColor(), max(0.0, dot(volumeNormal, GetSunLightDirection())));
                accumulatedColor += vec3(whiteWaterFactor) * (
                    diffuse +
                    shadowFactor * Specular(reflection, GetSunLightDirection(), 30.0) * GetSunLightColor() +
            		GetAmbientSkyColor());
            }
            #endif
            accumulatedColorMultiplier *= (1.0 - whiteWaterFactor);
            rayDirection = refract(rayDirection, volumeNormal, AIR_IOR / WaterIor); //공기에서 물로 진입 하는 굴절 
            
            accumulatedColor += accumulatedColorMultiplier * Specular(reflection, GetSunLightDirection(), waterShininess) * GetSunLightColor();
            accumulatedColor += accumulatedColorMultiplier * fresnelFactor * GetSkyColor(reflection); //물에 반사된 빛이 하늘을 향할거라는 가정
            accumulatedColorMultiplier *= (1.0 - fresnelFactor);
            
            // recalculate opaque depth now that the ray has been refracted
            t = IntersectOpaqueScene(rayOrigin, rayDirection, materialID);
            if( materialID != INVALID_OBJECT_ID )
            {
                opaquePosition = rayOrigin + t*rayDirection;
            }

            float volumeDepth = 0.0f;
            float signedDistance = 0.0;
            vec3 marchPosition = vec3(0);
            float minStepSize = SDF_START_STEP_SIZE;
            float minStepIncrement = float(SDF_END_STEP_SIZE - SDF_START_STEP_SIZE) / float(MAX_VOLUME_MARCH_STEPS);
            for(int i = 0; i < MAX_VOLUME_MARCH_STEPS; i++)
            {
                float marchSize = max(minStepSize, signedDistance);
				minStepSize += minStepIncrement;
                
                vec3 nextMarchPosition = rayOrigin + (volumeDepth + marchSize) * rayDirection;
                signedDistance = QueryOceanDistanceField(nextMarchPosition, sceneData.waterData.x);
                if(signedDistance > 0.0f)
                {
                    float start = 0.0;
                    float end = marchSize;

                    for(int j = 0; j < BINARY_SEARCH_STEPS; j++)
                    {
                        float midPoint = (start + end) * 0.5;
                        vec3 nextMarchPosition = rayOrigin + (volumeDepth + midPoint) * rayDirection;
                        float sdfValue = QueryVolumetricDistanceField(nextMarchPosition, sceneData.waterData.x);

                        // Use the SDF to nudget the mid point closer to the actual edge
                        midPoint = clamp(midPoint - sdfValue, start, end);
                        if(sdfValue > 0.0)
                        {
                            end = midPoint;
                        }
                        else
                        {
                            start = midPoint;
                        }
                    }
                    marchSize = end;
                }

                volumeDepth += marchSize;
                marchPosition = rayOrigin + volumeDepth*rayDirection;

                if(volumeDepth > t) //불투명 물체 만났음 바닥, 돌,...
                {
                    intersectFound = true;
                    volumeDepth = min(volumeDepth, t);
                    break;
                }

                vec3 previousLightFactor = accumulatedColorMultiplier;
                accumulatedColorMultiplier *= BeerLambert(vec3(WaterAbsorption) / WaterColor, marchSize);
                vec3 absorptionFromMarch = previousLightFactor - accumulatedColorMultiplier;

                accumulatedColor += accumulatedColorMultiplier * WaterColor * absorptionFromMarch * 
                    GetSunLightColor() * GetApproximateShadowFactor(marchPosition, GetSunLightDirection());
                accumulatedColor += accumulatedColorMultiplier * absorptionFromMarch * GetAmbientSkyColor();

                if(signedDistance > 0.0)//물밖으로 탈출
                {
                    intersectFound = true;
                    outsideVolume = true;
                    break;
                }
            }

            if(intersectFound && outsideVolume)
            {
                // Flip the normal since we're coming from inside the volume
                vec3 exitNormal = -GetVolumeNormal(marchPosition, sceneData.waterData.x, SCENE_TYPE_SIMPLIFIED_OCEAN);                    

                #if SECONDARY_REFLECTION
                float fresnelFactor = max(0.2, FresnelFactor(WaterIor, AIR_IOR, exitNormal, rayDirection));
                vec3 reflection = reflect(rayDirection, exitNormal);
                int reflectedMaterialID;
                float reflectionT = IntersectOpaqueScene(marchPosition, reflection, reflectedMaterialID);
                if( reflectedMaterialID != INVALID_OBJECT_ID )
                {
                    vec3 pos = marchPosition + reflection*reflectionT;
                    Material material = GetMaterial(reflectedMaterialID);
                    vec3 color = vec3(0);
                    CalculateLighting(pos,reflection, reflectedMaterialID, color, true);
                    accumulatedColor += accumulatedColorMultiplier * fresnelFactor * color;
                }
                else
                {
                    accumulatedColor += fresnelFactor * accumulatedColorMultiplier * GetSkyColor(rayDirection);
                }
                accumulatedColorMultiplier *= (1.0 - fresnelFactor);
                #endif
                
                rayDirection = refract(rayDirection, exitNormal, WaterIor / AIR_IOR);
                rayOrigin = marchPosition;
                t = IntersectOpaqueScene(marchPosition, rayDirection, materialID);
                if( materialID != INVALID_OBJECT_ID )
                {
                    opaquePosition = rayOrigin + t*rayDirection;
                }
                outsideVolume = true;
            }

            if(!intersectFound)
            {
                float t = GetApproximateIntersect(marchPosition, rayDirection);
                float halfT = t / 2.0;
                vec3 halfwayPosition = marchPosition + rayDirection * halfT;
                vec3 shadowFactor = GetApproximateShadowFactor(halfwayPosition, GetSunLightDirection());

                vec3 previousLightFactor = accumulatedColorMultiplier;
                accumulatedColorMultiplier *= BeerLambert(WaterAbsorption / WaterColor, t);
                vec3 absorptionFromMarch = previousLightFactor - accumulatedColorMultiplier;
                accumulatedColor += accumulatedColorMultiplier * WaterColor * shadowFactor * absorptionFromMarch * GetSunLightColor();
                accumulatedColor += accumulatedColorMultiplier * WaterColor * GetAmbientSkyColor() * absorptionFromMarch;

                volumeDepth += t;
                rayOrigin = rayOrigin + volumeDepth*rayDirection;
            }
        }
    }
    
    vec3 opaqueColor = vec3(0.0f);
    if(materialID != INVALID_OBJECT_ID)
    {
        CalculateLighting(opaquePosition,
                          rayDirection,
                          materialID, opaqueColor,
                          true);
        // opaqueColor = vec3(0.0f);
    }
    else
    {
        opaqueColor = GetSkyColor(rayDirection);
    }
    // return opaqueColor;
    return accumulatedColor + accumulatedColorMultiplier * opaqueColor;
}

float EncodeWaterColor()
{
    return float(
        int(WaterColor.r * 64.0) + 
        (int(WaterColor.g * 64.0) << 6) +
        (int(WaterColor.b * 64.0) << 12)); 
}

void DecodeWaterColor(float data)
{
    WaterColor.r = float(int(data) & 63) / 64.0;
    WaterColor.g = float((int(data) >> 6) & 63) / 64.0;
	WaterColor.b = float((int(data) >> 12) & 63) / 64.0;
}

void LoadConstants()
{
    // if(iFrame == 0 || KeyDown(82) || ALLOW_KEYBOARD_INPUT == 0)
    {
        WaterColor = vec3(0.02, 0.25, 0.53);
        WaterIor = 1.33; // Actual IOR of water
        WaterTurbulence = sceneData.waterData.y;
        WaterAbsorption = sceneData.waterData.z;
    }
    // else
    // {
    //     vec4 data = texelFetch(iChannel0, ivec2(0, 0), 0);
    //     WaterIor = data.r;
    //     WaterTurbulence = data.g;
    //     WaterAbsorption = data.b;
    //     DecodeWaterColor(data.a);
    // }
}
     
vec3 GammaCorrect(vec3 color) 
{
    return pow(color, vec3(1.0/2.2));
}

void main()
{
    LoadConstants();
    
    // Sacrafice 1 pixel so that we can save up updates from input
    // if(IsInputThread(fragCoord))
    // {
    //     ProcessInput();
    //     fragColor = vec4(WaterIor, WaterTurbulence, WaterAbsorption, EncodeWaterColor());
    //     return;
    // } 
    
    // float3 ray = -normalize(eyeWorld - input.posWorld);
    vec3 rayOrigin = sceneData.viewPos.xyz;
    vec3 rayDirection = normalize(inPos - rayOrigin);
    // rayDirection.y *= -1;
    vec3 color = Render(rayOrigin, rayDirection);
    outFragColor=vec4(GammaCorrect(color), 1.0 );
}

