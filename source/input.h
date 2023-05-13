//
// Created by Floogle on 07/05/2023.
//

#ifndef NESEMU_INPUT_H
#define NESEMU_INPUT_H

#ifdef _WIN32
#include "SDL.h"

uint8_t read_inputs()
{
    int numkeys;
    auto keys = SDL_GetKeyboardState(&numkeys);
    uint8_t result =
        (keys[SDL_SCANCODE_Z] << 0) | (keys[SDL_SCANCODE_X] << 1) | (keys[SDL_SCANCODE_RSHIFT] << 2) | (keys[SDL_SCANCODE_RETURN] << 3) | (keys[SDL_SCANCODE_UP] << 4) | (keys[SDL_SCANCODE_DOWN] << 5) | (keys[SDL_SCANCODE_LEFT] << 6) | (keys[SDL_SCANCODE_RIGHT] << 7);
    return result;
}
#endif // _WIN32

#endif // NESEMU_INPUT_H
