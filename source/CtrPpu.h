#ifndef NESEMU_3DSPPU_H
#define NESEMU_3DSPPU_H

#include <citro3d.h>
#include "Ppu.h"

struct vertex
{
    float x, y;
    float r, g, b;
    float u, v;
};

class CtrPpu : public Ppu
{
public:
    CtrPpu(std::shared_ptr<std::array<uint8_t, 0x2000>> chr, bool vertical);
    void render() override;

private:
    C3D_RenderTarget *target;
    DVLB_s *program_dvlb;
    shaderProgram_s program;
    int uLoc_projection;
    C3D_Mtx projection;
    C3D_Tex test_tex;
    vertex *vbuf;
};

#endif // NESEMU_3DSPPU_H
