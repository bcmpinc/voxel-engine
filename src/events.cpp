/*
    Voxel-Engine - A CPU based sparse octree renderer.
    Copyright (C) 2013  B.J. Conijn <bcmpinc@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <SDL/SDL.h>

#include "events.h"

// Buttons
class button {
    public:
    enum {
        W, A, S, D, Z, X, 
        SPACE, C, SHIFT,
        
        STATES
    };
};

static glm::dmat4 view;
    
// Quiting?
static bool button_state[button::STATES];
static bool mousemove=false;

// Position
static const double rotatespeed = -0.005, rollspeed = 0.05, movespeed = 1<<17;
static const int MILLISECONDS_PER_FRAME = 33;

bool quit  = false;
bool moves = true;
glm::dmat3 orientation;
glm::dvec3 position;

// checks user input
void handle_events() {
    moves=false;

    SDL_Event event;
  
    /* Check for events */
    while (SDL_PollEvent (&event)) {
        switch (event.type) {
        case SDL_KEYUP:
        case SDL_KEYDOWN: {
            bool state = (event.type == SDL_KEYDOWN);
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    quit = true;
                    break;
                case SDLK_w:
                    button_state[button::W] = state;
                    break;
                case SDLK_a:
                    button_state[button::A] = state;
                    break;
                case SDLK_s:
                    button_state[button::S] = state;
                    break;
                case SDLK_d:
                    button_state[button::D] = state;
                    break;
                case SDLK_c:
                    button_state[button::C] = state;
                    break;
                case SDLK_z:
                    button_state[button::Z] = state;
                    break;
                case SDLK_x:
                    button_state[button::X] = state;
                    break;
                case SDLK_SPACE:
                    button_state[button::SPACE] = state;
                    break;
                case SDLK_LSHIFT:
                    button_state[button::SHIFT] = state;
                    break;
                default:
                    break;
            }
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            SDL_ShowCursor(mousemove);
            mousemove=!mousemove;
            SDL_WM_GrabInput(mousemove?SDL_GRAB_ON:SDL_GRAB_OFF);
            break;
        }
        case SDL_MOUSEMOTION: {
            if (mousemove) {
                glm::dmat4 tview = glm::transpose(view);
                view = glm::rotate(
                    view, 
                    rotatespeed * event.motion.xrel,
                    glm::dvec3(tview[1])
                );
                view = glm::rotate(
                    view, 
                    rotatespeed * event.motion.yrel,
                    glm::dvec3(tview[0])
                );
                orientation = glm::dmat3(view);                
                moves=true;
            }
            break;
        }
        case SDL_QUIT:
            quit = true;
            break;
        default:
            break;
        }
    }

    double dist = movespeed;
    if (button_state[button::SHIFT]) {
        dist *= 16;
    }
    
    glm::dmat3 M = glm::transpose(orientation);
    
    int rd = 0;
    if (button_state[button::Z]) rd++;
    if (button_state[button::X]) rd--;
    if (rd) {
        glm::dmat4 tview = glm::transpose(view);
        view = glm::rotate(
            view, 
            rd*rollspeed,
            glm::dvec3(tview[2])
        );
        orientation = glm::dmat3(view);                
        moves=true;
    }
    if (button_state[button::W]) {
        position += dist * M[2];
        moves=true;
    }
    if (button_state[button::S]) {
        position -= dist * M[2];
        moves=true;
    }
    if (button_state[button::A]) {
        position -= dist * M[0];
        moves=true;
    }
    if (button_state[button::D]) {
        position += dist * M[0];
        moves=true;
    }
    if (button_state[button::C]) {
        position -= dist * M[1];
        moves=true;
    }
    if (button_state[button::SPACE]) {
        position += dist * M[1];
        moves=true;
    }
} 

void next_frame(int elapsed) {
    int delay = MILLISECONDS_PER_FRAME-elapsed;
    if (delay>10) {
        SDL_Delay(delay);
    }    
}
