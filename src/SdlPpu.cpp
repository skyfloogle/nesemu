//
// Created by Floogle on 07/05/2023.
//

#include "SdlPpu.h"
#include "palette.h"
#include <windows.h>

SdlPpu::SdlPpu(std::shared_ptr<std::array<uint8_t, 0x2000>> chr, bool vertical) : Ppu(std::move(chr), vertical) {
    SetProcessDPIAware();
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    window = SDL_CreateWindow(
            "DK",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            768, 720, SDL_WINDOW_SHOWN
            );
    window_surface = SDL_GetWindowSurface(window);
    game_surface = SDL_CreateRGBSurfaceWithFormat(0, 256, 240, 32, window_surface->format->format);
    last_timestamp = SDL_GetPerformanceCounter();
}

void SdlPpu::render() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            exit(0);
        }
    }
    SDL_FillRect(game_surface, nullptr, palette[palettes[0]]);
    SDL_LockSurface(game_surface);
    Ppu::render(static_cast<uint32_t *>(game_surface->pixels));
    SDL_UnlockSurface(game_surface);
    SDL_BlitScaled(game_surface, nullptr, window_surface, nullptr);
    SDL_UpdateWindowSurface(window);
    // timing
    Uint32 new_timestamp = SDL_GetPerformanceCounter();
    double delay = floor(16.666 - double(new_timestamp - last_timestamp) / double(SDL_GetPerformanceFrequency()));
    SDL_Delay(max(0.0, delay));
    last_timestamp = new_timestamp;
}