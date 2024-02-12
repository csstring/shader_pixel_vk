#include "Portal.hpp"
#include "Camera.hpp"
#include <glm/gtx/transform.hpp>
#include "glm/gtx/string_cast.hpp"

void Portal::update()
{
  Camera& cam = Camera::getInstance();
  static CAMERA_MOVE_DIR beforeDir = cam._moveDir;

  if (cam._cameraPos.x < xmin || cam._cameraPos.x > xmax 
    ||cam._cameraPos.y < ymin || cam._cameraPos.y > ymax
    ||cam._cameraPos.z < zmin || cam._cameraPos.z > zmax){
    beforeDir = cam._moveDir;
    return;
  }
  CAMERA_MOVE_DIR curDir = cam._moveDir == CAMERA_MOVE_DIR::MOVE_STOP ? beforeDir : cam._moveDir;
  state = state == PortalState::In_World1 ? PortalState::In_World2 : PortalState::In_World1;
  switch (curDir)
  {
    // std::cout << glm::to_string(cam._cameraFront) << std::endl;
    case CAMERA_MOVE_DIR::MOVE_W:
        cam._cameraPos += cam._cameraFront * 3.f;
        break;
    case CAMERA_MOVE_DIR::MOVE_S:
        cam._cameraPos -= cam._cameraFront * 3.f;
        break;
    case CAMERA_MOVE_DIR::MOVE_A:
        cam._cameraPos -= cam._cameraRight * 3.f;
        break;
    case CAMERA_MOVE_DIR::MOVE_D:
        cam._cameraPos += cam._cameraRight * 3.f;
        break;    
    default:
        break;
  }
  beforeDir = cam._moveDir;
}

void Portal::initialize(glm::mat4 scale, glm::mat4 translate, PortalState state)
{
  this->translate = translate;
  this->scale = scale;
  this->state = state;
  xmin = (scale * translate * glm::vec4(-0.5,0,0,1)).x;
  xmax = (scale * translate * glm::vec4(0.5,0,0,1)).x;
  ymin = (scale * translate * glm::vec4(0,-0.5,0,1)).y;
  ymax = (scale * translate * glm::vec4(0,0.5,0,1)).y;
  zmin = (translate * glm::vec4(0,0,-0.5,1)).z;
  zmax = (translate * glm::vec4(0,0,0.5,1)).z;
  Camera& cam = Camera::getInstance();
  std::cout << "xmin : " << xmin << "xmax : " << xmax << "ymin : " << ymin << "ymax : " << ymax << "zmin : " << zmin << "zmax : " << zmax << std::endl;
  if (cam._cameraPos.x < xmin || cam._cameraPos.x > xmax 
    ||cam._cameraPos.y < ymin || cam._cameraPos.y > ymax
    ||cam._cameraPos.z < zmin || cam._cameraPos.z > zmax){
    inPortal = false;
  } else {
    inPortal = true;
  }
}