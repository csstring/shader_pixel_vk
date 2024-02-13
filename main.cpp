#include "vk_engine.hpp"
#include <iostream>
#include "Camera.hpp"
#include <iostream>
#include "glm/glm.hpp"

#include <algorithm>

#define UI0 1597334673U
#define UI1 3812015801U
#define UI2 glm::uvec2(UI0, UI1)
#define UI3 glm::uvec3(UI0, UI1, 2798796415U)
#define UIF (1.0f / float(0xffffffffU))

glm::vec3 hash33(glm::vec3 p)
{
	glm::uvec3 q = glm::uvec3(glm::ivec3(p)) * UI3;
	q = (q.x ^ q.y ^ q.z)*UI3;
	return -1.f + 2.f * glm::vec3(q) * UIF;
}

float remap(float x, float a, float b, float c, float d)
{
    return (((x - a) / (b - a)) * (d - c)) + c;
}

// Gradient noise by iq (modified to be tileable)
float gradientNoise(glm::vec3 x, float freq)
{
    // grid
    glm::vec3 p = floor(x);
    glm::vec3 w = fract(x);
    
    // quintic interpolant
    glm::vec3 u = w * w * w * (w * (w * 6.f - 15.f) + 10.f);

    
    // gradients
    glm::vec3 ga = hash33(mod(p + glm::vec3(0., 0., 0.), freq));
    glm::vec3 gb = hash33(mod(p + glm::vec3(1., 0., 0.), freq));
    glm::vec3 gc = hash33(mod(p + glm::vec3(0., 1., 0.), freq));
    glm::vec3 gd = hash33(mod(p + glm::vec3(1., 1., 0.), freq));
    glm::vec3 ge = hash33(mod(p + glm::vec3(0., 0., 1.), freq));
    glm::vec3 gf = hash33(mod(p + glm::vec3(1., 0., 1.), freq));
    glm::vec3 gg = hash33(mod(p + glm::vec3(0., 1., 1.), freq));
    glm::vec3 gh = hash33(mod(p + glm::vec3(1., 1., 1.), freq));
    
    // projections
    float va = dot(ga, w - glm::vec3(0., 0., 0.));
    float vb = dot(gb, w - glm::vec3(1., 0., 0.));
    float vc = dot(gc, w - glm::vec3(0., 1., 0.));
    float vd = dot(gd, w - glm::vec3(1., 1., 0.));
    float ve = dot(ge, w - glm::vec3(0., 0., 1.));
    float vf = dot(gf, w - glm::vec3(1., 0., 1.));
    float vg = dot(gg, w - glm::vec3(0., 1., 1.));
    float vh = dot(gh, w - glm::vec3(1., 1., 1.));
	
    // interpolation
    return va + 
           u.x * (vb - va) + 
           u.y * (vc - va) + 
           u.z * (ve - va) + 
           u.x * u.y * (va - vb - vc + vd) + 
           u.y * u.z * (va - vc - ve + vg) + 
           u.z * u.x * (va - vb - ve + vf) + 
           u.x * u.y * u.z * (-va + vb + vc - vd + ve - vf - vg + vh);
}

// Tileable 3D worley noise
float worleyNoise(glm::vec3 uv, float freq)
{    
    glm::vec3 id = floor(uv);
    glm::vec3 p = fract(uv);
    
    float minDist = 10000.;
    for (float x = -1.; x <= 1.; ++x)
    {
        for(float y = -1.; y <= 1.; ++y)
        {
            for(float z = -1.; z <= 1.; ++z)
            {
              glm::vec3 offset = glm::vec3(x, y, z);
            	glm::vec3 h = hash33(mod(id + offset, glm::vec3(freq))) * .5f + .5f;
    			    h += offset;
            	glm::vec3 d = p - h;
           		minDist = glm::min(minDist, glm::dot(d, d));
            }
        }
    }
    
    // inverted worley noise
    return 1. - minDist;
}

float perlinfbm(glm::vec3 p, float freq, int octaves)
{
    float G = exp2(-.85);
    float amp = 1.;
    float noise = 0.;
    for (int i = 0; i < octaves; ++i)
    {
        noise += amp * gradientNoise(p * freq, freq);
        freq *= 2.;
        amp *= G;
    }
    
    return noise;
}

float worleyFbm(glm::vec3 p, float freq)
{
    return worleyNoise(p*freq, freq) * .625 +
        	 worleyNoise(p*freq*2.f, freq*2.f) * .25 +
        	 worleyNoise(p*freq*4.f, freq*4.f) * .125;
}

float cloudDensity(glm::vec3 uvw)
{
    float freq = 4.0f;

    float pfbm = glm::mix(1.f, perlinfbm(uvw, 4., 7), .5f);
    pfbm = abs(pfbm * 2.f - 1.f); // billowy perlin noise

    float g = worleyFbm(uvw, freq);
    float r = remap(pfbm, 0.f, 1.f, g, 1.f); // perlin-worley
    float b = worleyFbm(uvw, freq * 2.0f);
    float a = worleyFbm(uvw, freq * 4.0f);

    float perlinWorley = r;
    float wfbm = g * .625f + b * .125f + a * .25f;

    // cloud shape modeled after the GPU Pro 7 chapter
    float cloud = remap(perlinWorley, wfbm - 1., 1., 0., 1.);
    cloud = remap(cloud, .85, 1., 0., 1.); // fake cloud coverage
    
    return std::clamp(cloud, .0f, 1.0f);
}

void check(){
  system("leaks a.out");
}

int main()
{
  // for (int i =0; i < 16; ++i){
  //   for (int j =0; j < 16; ++j){
  //     for (int k = 0; k < 16; ++k){
  //       std::cout << cloudDensity(glm::vec3(i,j,k)) << ' ';
  //     }
  //     std::cout << std::endl;
  //   }
  // }
  VulkanEngine engine;
  Camera& _camera = Camera::getInstance();
  _camera.initialize();
  engine.init();
  engine.run();
  engine.cleanup();
  return 0;
}
