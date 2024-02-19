#pragma once
#include "vk_types.hpp"
#include <glm/glm.hpp>

class Portal
{
  private:
    PortalState state;
    glm::mat4 scale;
    glm::mat4 translate;
    float xmin, xmax, ymin, ymax, zmin, zmax;
    bool inPortal;
  public:
    Portal(){};
    ~Portal(){};
    void initialize(glm::mat4 scale, glm::mat4 translate, PortalState state);
    void update();
    PortalState getPortalState(){return state;};
    glm::mat4 getModelTransForm(){return scale * translate;};
};
