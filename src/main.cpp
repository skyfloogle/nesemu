#include <iostream>
#include "Cpu.h"
#include <cstdio>
#include <utility>

int main() {
    auto prg = std::make_shared<std::array<uint8_t, 0x8000>>();
    auto chr = std::make_shared<std::array<uint8_t, 0x2000>>();
    FILE *f;
    fopen_s(&f, "../Donkey Kong (World) (Rev A).nes", "rb");
    fseek(f, 4, 0);
    unsigned char prg_size;
    fread(&prg_size, 1, 1, f);
    fseek(f, 16, 0);
    fread(&(*prg)[0], 16384, std::min(2u, size_t(prg_size)), f);
    fread(&(*chr)[0], 1, 8192, f);
    fclose(f);
    for (int i = prg_size; i < 2; i += prg_size) {
        memcpy(&(*prg)[i * 16384], &(*prg)[0], 16384 * prg_size);
    }
    Cpu cpu(prg, chr);
    cpu.reset();
    int cycles = 0;
    while (true) {
        cycles += cpu.run_instruction();
        if (cpu.just_wrote_oamdma) {
            cycles += 514;
            cpu.just_wrote_oamdma = false;
        }
        if (cycles >= 29780) {
            cycles -= 29780;
            cpu.vblank();
        }
    }
    return 0;
}
