#include "Portal.hpp"
#include "Camera.hpp"
#include <glm/gtx/transform.hpp>
#include "glm/gtx/string_cast.hpp"
#include "vk_engine.hpp"

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
  state = state == PortalState::In_World0 ? PortalState::In_World1 : PortalState::In_World0;
  switch (curDir)
  {
    // std::cout << glm::to_string(cam._cameraFront) << std::endl;
    case CAMERA_MOVE_DIR::MOVE_W:
        cam._cameraPos += cam._cameraFront * 4.f;
        break;
    case CAMERA_MOVE_DIR::MOVE_S:
        cam._cameraPos -= cam._cameraFront * 4.f;
        break;
    case CAMERA_MOVE_DIR::MOVE_A:
        cam._cameraPos -= cam._cameraRight * 4.f;
        break;
    case CAMERA_MOVE_DIR::MOVE_D:
        cam._cameraPos += cam._cameraRight * 4.f;
        break;    
    default:
        break;
  }
  beforeDir = cam._moveDir;
}

void Portal::initialize(glm::mat4 scale, glm::mat4 rotation, glm::mat4 translate, PortalState state)
{
  this->translate = translate;
  this->scale = scale;
  this->state = state;
  this->rot = rotation;
  xmin = (translate * rotation * scale * glm::vec4(-0.5,0,0,1)).x;
  xmax = (translate * rotation * scale * glm::vec4(0.5,0,0,1)).x;
  ymin = (translate * rotation * scale * glm::vec4(0,-0.5,0,1)).y;
  ymax = (translate * rotation * scale * glm::vec4(0,0.5,0,1)).y;
  zmin = (translate * rotation * glm::vec4(0,0,-1.3,1)).z;
  zmax = (translate * rotation * glm::vec4(0,0,1.3,1)).z;
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

void Portal::loadSceneObject(VulkanEngine* engine)
{
  auto StencilFill_Zero = loadGltf(engine,"./assets/models/", "plane_z.gltf", MaterialPass::StencilFill_Zero); //_portalManager
	auto StencilFill_One = loadGltf(engine,"./assets/models/", "plane_z.gltf", MaterialPass::StencilFill_One);//_portalManager
  auto julia = loadGltf(engine,"./assets/models/", "cube.gltf", MaterialPass::Julia, glm::scale(glm::vec3(0.2f)));

  assert(StencilFill_Zero.has_value());
	assert(StencilFill_One.has_value());
  assert(julia.has_value());
  
  engine->loadedScenes["StencilFill_Zero"] = *StencilFill_Zero;
	engine->loadedScenes["StencilFill_One"] = *StencilFill_One;
  engine->loadedScenes["julia"] = *julia;
}

void Portal::drawSceneObject(VulkanEngine* engine)
{
  switch (getPortalState())
	{
	case PortalState::In_World0:
		engine->loadedScenes["StencilFill_One"]->Draw(getModelTransForm(), engine->mainDrawContext);
		break;
	case PortalState::In_World1:
		engine->loadedScenes["StencilFill_Zero"]->Draw(getModelTransForm(), engine->mainDrawContext);
		// loadedScenes["julia"]->Draw(glm::scale(glm::mat4(1.0f), glm::vec3(40.0f)) ,mainDrawContext);
		break;
	default:
		break;
	}
}