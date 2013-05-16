#include <glm/gtc/matrix_transform.hpp>
#include <SDL/SDL.h>

#include "events.h"

// Buttons
class button {
    public:
    enum {
        W, A, S, D,
        SPACE, C, SHIFT,
        
        STATES
    };
};

namespace {
    glm::dmat4 view;
      
    // Quiting?
    bool button_state[button::STATES];
    bool mousemove=false;

    // Position
    const double rotatespeed = -0.3, movespeed = 1<<12;  
    const int MILLISECONDS_PER_FRAME = 50;
}

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
