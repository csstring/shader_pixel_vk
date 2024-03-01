#include "SDL_Event.hpp"
#include "SDL.h"
#include "SDL_vulkan.h"
#include "Camera.hpp"
#include "imgui_impl_sdl2.h"
namespace vkutil {
  void SDL_event_process(SDL_Event* e, Camera& camera, bool& bQuit)
  {
    while (SDL_PollEvent(e) != 0)
		{
      ImGui_ImplSDL2_ProcessEvent(e);
			if (e->type == SDL_QUIT || (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_ESCAPE)) 
        bQuit = true;
			else if (e->type == SDL_MOUSEMOTION)
			{
        camera.mouse_callback(e->motion.x, e->motion.y);
			}
      else if (e->type == SDL_MOUSEBUTTONDOWN){
        camera._clickOn = true;
      }
      else if (e->type == SDL_MOUSEBUTTONUP){
        camera._clickOn = false;
      }
      else if (e->type == SDL_KEYDOWN)
      {
        if (e->key.keysym.sym == SDLK_c){
          camera._isOn = camera._isOn == false ? true : false;
        }
				if (e->key.keysym.sym == SDLK_w){
					camera._moveDir = CAMERA_MOVE_DIR::MOVE_W;
				}
				else if (e->key.keysym.sym == SDLK_s){
					camera._moveDir = CAMERA_MOVE_DIR::MOVE_S;
				}
				else if (e->key.keysym.sym == SDLK_a){
					camera._moveDir = CAMERA_MOVE_DIR::MOVE_A;
				}
				else if (e->key.keysym.sym == SDLK_d){
					camera._moveDir = CAMERA_MOVE_DIR::MOVE_D;
				}
      }
      else if (e->type == SDL_KEYUP){
        if (e->key.keysym.sym == SDLK_w || e->key.keysym.sym == SDLK_s
              || e->key.keysym.sym == SDLK_a||e->key.keysym.sym == SDLK_d){
          camera._moveDir = CAMERA_MOVE_DIR::MOVE_STOP;
        }
      }
    }
  }
}