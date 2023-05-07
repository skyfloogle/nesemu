//
// Created by Floogle on 07/05/2023.
//

#include "SdlPpu.h"
#include "palette.h"
#include <windows.h>

SdlPpu::SdlPpu(std::shared_ptr<std::array<uint8_t, 0x2000>> chr) : Ppu(std::move(chr)) {
    SetProcessDPIAware();
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow(
            "NES",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            256, 240, SDL_WINDOW_SHOWN
            );
    surface = SDL_GetWindowSurface(window);
}

void SdlPpu::render() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            exit(0);
        }
    }
    SDL_FillRect(surface, nullptr, palette[palettes[0]]);
    SDL_LockSurface(surface);
    Ppu::render(static_cast<uint32_t *>(surface->pixels));
    SDL_UnlockSurface(surface);
    SDL_UpdateWindowSurface(window);
    SDL_Delay(16);
}