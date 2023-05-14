#include <iostream>
#include "Cpu.h"
#include <cstdio>
#include <utility>
#include <memory>
#include "dk_nes.h"

#ifdef _WIN32
#include "SdlPpu.h"
#else
#include "CtrPpu.h"
#endif

#include <3ds.h>

int main(int argc, char *argv[])
{
    auto prg = std::make_shared<std::array<uint8_t, 0x8000>>();
    auto chr = std::make_shared<std::array<uint8_t, 0x2000>>();
    uint8_t prg_size = dk_nes[4];
    uint8_t flags6 = dk_nes[6];
    memcpy(&(*prg)[0], &dk_nes[16], 16384 * std::min(2u, size_t(prg_size)));
    memcpy(&(*chr)[0], &dk_nes[16 + 16384 * std::min(2u, size_t(prg_size))], 8192);
    for (int i = prg_size; i < 2; i += prg_size)
    {
        memcpy(&(*prg)[i * 16384], &(*prg)[0], 16384 * prg_size);
    }
    std::unique_ptr<Ppu> ppu(new CtrPpu(chr, (flags6 & 1) != 0));
    Cpu cpu(prg, std::move(ppu));
    cpu.reset();
    int cycles = 0;
    while (true)
    {
        cycles += cpu.run_instruction();
        if (cpu.just_wrote_oamdma)
        {
            cycles += 514;
            cpu.just_wrote_oamdma = false;
        }
        if (cycles >= 29780)
        {
            cycles -= 29780;
            cpu.vblank();
        }
    }
    return 0;
}
