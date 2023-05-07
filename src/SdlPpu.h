//
// Created by Floogle on 07/05/2023.
//

#ifndef NESEMU_SDLPPU_H
#define NESEMU_SDLPPU_H

#include "Ppu.h"
#include "SDL.h"


class SdlPpu : public Ppu {
public:
    SdlPpu(std::shared_ptr<std::array<uint8_t, 0x2000>> chr);
    void render() override;
private:
    SDL_Window* window;
    SDL_Surface* surface;

};


#endif //NESEMU_SDLPPU_H
