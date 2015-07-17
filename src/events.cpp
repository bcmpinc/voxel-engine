/*
    Voxel-Engine - A CPU based sparse octree renderer.
    Copyright (C) 2013,2015  B.J. Conijn <bcmpinc@users.sourceforge.net>

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
#include <SDL2/SDL.h>

#include "events.h"

// Buttons
class button {
    public:
    enum {
        FORWARD, BACKWARD,
        LEFT, RIGHT,
        UP, DOWN,
        ROLL_LEFT, ROLL_RIGHT,
        FAST,        
        STATES
    };
};

static glm::dmat4 view;
    
static bool button_state[button::STATES];
static bool mousemove=false;
static int jdx=0, jdy=0, jvx=0, jvy=0;

// Position
static const double rotatespeed = -0.005, rollspeed = 0.05, movespeed = 1<<17;
static const int MILLISECONDS_PER_FRAME = 33;
static const int DEADZONE = 1500;

bool quit  = false;
bool moves = true;
glm::dmat3 orientation;
glm::dvec3 position;

static void attach_joysticks() {
    static int joy_error = SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    if (joy_error) return;
    // Check for joystick
    int n = SDL_NumJoysticks();
    printf("Attaching %d joysticks\n", n);
    for (int i=0; i<n; i++) {
        // Open joystick
        SDL_Joystick * joy = SDL_JoystickOpen(0);

        if (joy) {
            printf("Opened Joystick %d\n", i);
            printf("Name: %s\n", SDL_JoystickName(joy));
            printf("Number of Axes: %d\n", SDL_JoystickNumAxes(joy));
            printf("Number of Buttons: %d\n", SDL_JoystickNumButtons(joy));
            printf("Number of Balls: %d\n", SDL_JoystickNumBalls(joy));
        } else {
            printf("Couldn't open Joystick %d\n", i);
        }
    }
}

// checks user input
void handle_events() {
    static bool first_call = true;
    if (first_call) {
        attach_joysticks();
        first_call = false;
    }
    
    moves=false;

    SDL_Event event;
    
    /* Check for events */
    while (SDL_PollEvent (&event)) {
        switch (event.type) {
        case SDL_KEYUP:
        case SDL_KEYDOWN: {
            bool state = (event.type == SDL_KEYDOWN);
            switch (event.key.keysym.scancode) {
                case SDL_SCANCODE_ESCAPE:
                    quit = true;
                    break;
                case SDL_SCANCODE_W:
                    button_state[button::FORWARD] = state;
                    break;
                case SDL_SCANCODE_S:
                    button_state[button::BACKWARD] = state;
                    break;
                case SDL_SCANCODE_A:
                    button_state[button::LEFT] = state;
                    break;
                case SDL_SCANCODE_D:
                    button_state[button::RIGHT] = state;
                    break;
                case SDL_SCANCODE_SPACE:
                    button_state[button::UP] = state;
                    break;
                case SDL_SCANCODE_LCTRL:
                    button_state[button::DOWN] = state;
                    break;
                case SDL_SCANCODE_Q:
                    button_state[button::ROLL_LEFT] = state;
                    break;
                case SDL_SCANCODE_E:
                    button_state[button::ROLL_RIGHT] = state;
                    break;
                case SDL_SCANCODE_LSHIFT:
                    button_state[button::FAST] = state;
                    break;
                default:
                    if (state && event.key.keysym.sym == SDLK_j) {
                        attach_joysticks();
                    }
                    break;
            }
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            mousemove=!mousemove;
            SDL_SetRelativeMouseMode(mousemove?SDL_TRUE:SDL_FALSE);
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
        case SDL_JOYAXISMOTION: {
            switch (event.jaxis.axis) {
                case 0: jdx = event.jaxis.value; break;
                case 1: jdy = event.jaxis.value; break;
                case 2: jvx = event.jaxis.value; break;
                case 3: jvy = event.jaxis.value; break;
                default: break;
            }
            break;
        }
        case SDL_JOYBUTTONUP: 
        case SDL_JOYBUTTONDOWN: {
            bool state = (event.type == SDL_JOYBUTTONDOWN);
            switch (event.jbutton.button) {
                case 1: 
                    button_state[button::FAST] = state;
                    break;
                case 2: 
                    button_state[button::UP] = state;
                    break;
                case 3: 
                    button_state[button::DOWN] = state;
                    break;
                case 4:
                    button_state[button::ROLL_LEFT] = state;
                    break;
                case 5:
                    button_state[button::ROLL_RIGHT] = state;
                    break;
                default:
                    break;
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

    if (jvx < -DEADZONE || DEADZONE < jvx || jvy < -DEADZONE || DEADZONE < jvy) {
        glm::dmat4 tview = glm::transpose(view);
        view = glm::rotate(
            view, 
            rotatespeed * jvx / 500.0,
            glm::dvec3(tview[1])
        );
        view = glm::rotate(
            view, 
            rotatespeed * jvy / -500.0,
            glm::dvec3(tview[0])
        );
        orientation = glm::dmat3(view);                
        moves=true;
    }
    
    double dist = movespeed;
    if (button_state[button::FAST]) {
        dist *= 16;
    }
    
    glm::dmat3 M = glm::transpose(orientation);
    
    int rd = 0;
    if (button_state[button::ROLL_LEFT]) rd++;
    if (button_state[button::ROLL_RIGHT]) rd--;
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
    if (button_state[button::FORWARD]) {
        position += dist * M[2];
        moves=true;
    }
    if (button_state[button::BACKWARD]) {
        position -= dist * M[2];
        moves=true;
    }
    if (jdx < -DEADZONE || DEADZONE < jdx || jdy < -DEADZONE || DEADZONE < jdy) {
        double h = dist * hypot(jdx, jdy) / 100000000.;
        position += jdx * h * M[0];
        position -= jdy * h * M[2];
        moves=true;
    }
    if (button_state[button::LEFT]) {
        position -= dist * M[0];
        moves=true;
    }
    if (button_state[button::RIGHT]) {
        position += dist * M[0];
        moves=true;
    }
    if (button_state[button::UP]) {
        position += dist * M[1];
        moves=true;
    }
    if (button_state[button::DOWN]) {
        position -= dist * M[1];
        moves=true;
    }
} 

void next_frame(int elapsed) {
    int delay = MILLISECONDS_PER_FRAME-elapsed;
    if (delay>10) {
        SDL_Delay(delay);
    }    
}
