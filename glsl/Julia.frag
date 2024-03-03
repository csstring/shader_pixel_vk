#version 460

layout(set = 0, binding = 0) uniform  SceneData{   
	vec4 ambientColor;
	vec4 sunlightDirection; //w for sun power
	vec4 sunlightColor;
	vec4 viewPos;
  vec4 Data; //a.time, b.WaterTurbulence c.WaterAbsorption d.color
  vec4 cloudData; //cloud absortion, anois, 
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

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inModelPos;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inPos;
layout (location = 4) in mat4 invModel;

layout (location = 0) out vec4 outFragColor;

vec2 isphere( in vec4 sph, in vec3 ro, in vec3 rd )
{
    vec3 oc = ro - sph.xyz;
    
	  float b = dot(oc,rd);
	  float c = dot(oc,oc) - sph.w*sph.w;
    float h = b*b - c;
    if( h<0.0 ) return vec2(-1.0);
    h = sqrt( h );
    return -b + vec2(-h,h);
}

float map( in vec3 p, in vec3 c, out vec4 resColor )
{
  vec3 z = p;
  float m = dot(z,z);

  vec4 trap = vec4(abs(z),m);
	float dz = 1.0;
    
	for( int i=0; i<4; i++ )
  {
	  dz = 8.0*pow(m,3.5)*dz;
    float r = length(z);
    float b = 8.0*acos( clamp(z.y/r, -1.0, 1.0));
    float a = 8.0*atan( z.x, z.z );
    z = c + pow(r,10.0) * vec3( sin(b)*sin(a), cos(b), sin(b)*cos(a) );
      
    trap = min( trap, vec4(abs(z),m) );
    m = dot(z,z);
		if( m > 2.0 )
      break;
    }

    resColor = trap;

    return 0.25*log(m)*sqrt(m)/dz;
}

float raycast( in vec3 ro, in vec3 rd, out vec4 rescol, float fov, vec3 c )
{
  float res = -1.0;
  vec2 dis = isphere( vec4( 0.0, 0.0, 0.0, 1.25), ro, rd );
  if( dis.y<0.0 )
    return -1.0;
  dis.x = max( dis.x, 0.0 );

	vec4 trap;
	float t = dis.x;
	for( int i=0; i<32; i++  )
  { 
    vec3 p = ro + rd*t;
		float dt = map( p, c, trap );
		if( t>dis.y || dt<1e-3 ) break;
    t += min(dt,0.05);
  }
    
  if( t<dis.y )
  {
    rescol = trap;
    res = t;
  }

  return res;
}

vec3 GetNormal( in vec3 pos, vec3 c )
{
  vec4 tmp;
  vec3 n = vec3(0.0);
  for( int i= 0; i<4; i++ )
  {
    vec3 e = 0.5773*(2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
    n += e*map(pos+0.0005*e, c, tmp);
  }
  return normalize(n);
}

void main( )
{
  float time = sceneData.Data.x * 0.1;
  float fov = 1.5;
  vec3 rayOrigin = (invModel * sceneData.viewPos).xyz;
  vec3 rayDirection = normalize(inPos - rayOrigin);

	vec3 light1 = normalize(sceneData.sunlightDirection.xyz);
	vec3 light2 = sceneData.sunlightDirection.xyz;
  vec3 cc = vec3( sin(time * 1.7321/2.f), cos(time * 1.618), sin(time * 1.303) );
	vec3 cc4 = vec3( 0.9*cos(3.9+1.2*time)-.3, 0.8*cos(2.5+1.1*time), 0.8*cos(3.4+1.3*time) );
	if( length(cc)<0.50 ) cc=0.50*normalize(cc);
	if( length(cc)>0.95 ) cc=0.95*normalize(cc);

	vec3 col = vec3(1.0,1.0,1.0)*0.3;//color
	vec4 tra;
  float t = raycast( rayOrigin, rayDirection, tra, fov, cc );
  if( t<0.0 )
    discard;

  
	{
		vec3 pos = rayOrigin + t*rayDirection;
    vec3 nor = GetNormal(pos, cc);
    vec3 hal = normalize( light1-rayDirection);
        
    col = vec3(1.0,1.0,1.0)*0.1;
    col = mix( col, vec3(0.7,0.3,0.3), sqrt(tra.x) );
		col = mix( col, vec3(1.0,0.5,0.2), sqrt(tra.y) );
		col = mix( col, vec3(1.0,1.0,1.0), tra.z );
    col *= 0.4;
                
		float dif1 = clamp( dot( light1, nor ), 0.0, 1.0 );
    float fre = 0.04 + 0.96*pow( clamp(1.0-dot(-rayDirection,nor),0.0,1.0), 5.0 );
    float spe = pow( clamp(dot(nor,hal),0.0,1.0), 12.0 ) * dif1 * fre*8.0;
        
		vec3 lin  = 1.0*vec3(0.15,0.20,0.23)*(0.6+0.4*nor.y)*(0.1);
		lin += 4.0*vec3(1.00,0.90,0.60)*dif1;

    lin += 2.0*vec3(1.00,1.00,1.00)*spe;
    lin += 0.3*vec3(0.20,0.30,0.40)*(0.02);
		col *= lin;
    col += spe;
	}

	col = sqrt( col );

	outFragColor = vec4( col, 1.0 );
}