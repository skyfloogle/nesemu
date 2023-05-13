#ifndef NESEMU_3DSPPU_H
#define NESEMU_3DSPPU_H

#include <citro3d.h>
#include "Ppu.h"

class CtrPpu : public Ppu
{
public:
    CtrPpu(std::shared_ptr<std::array<uint8_t, 0x2000>> chr, bool vertical);
    void render() override;

private:
    C3D_RenderTarget *target;
};

#endif // NESEMU_3DSPPU_H
