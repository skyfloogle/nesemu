#include "CtrPpu.h"
#include <3ds.h>
#include <citro3d.h>
#include <cstdio>
#include "vshader_shbin.h"

#include "palette.h"

#define DISPLAY_TRANSFER_FLAGS                                                                     \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |               \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

CtrPpu::CtrPpu(std::shared_ptr<std::array<uint8_t, 0x2000>> chr, bool vertical) : Ppu(chr, vertical)
{
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE * 3);
    target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH16);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    consoleInit(GFX_BOTTOM, nullptr);
    puts("Hello!");

    // Load the vertex shader, create a shader program and bind it
    vshader_dvlb = DVLB_ParseFile((u32 *)vshader_shbin, vshader_shbin_size);
    shaderProgramInit(&program);
    shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);
    C3D_BindProgram(&program);

    // Get the location of the uniforms
    uLoc_projection = shaderInstanceGetUniformLocation(program.vertexShader, "projection");

    // Configure attributes for use with the vertex shader
    C3D_AttrInfo *attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddFixed(attrInfo, 0);
    AttrInfo_AddFixed(attrInfo, 1);

    // Compute the projection matrix
    Mtx_OrthoTilt(&projection, 0.0, 256.0, 0.0, 240.0, 0.0, 1.0, true);

    C3D_TexInitParams params;
    params.width = 512;
    params.height = 256;
    params.format = GPU_A8;
    params.type = GPU_TEX_2D;
    params.onVram = true;
    params.maxLevel = 0;
    C3D_TexInitWithParams(&test_tex, nullptr, params);

    u32 size;
    u8 *texdata = (u8 *)C3D_Tex2DGetImagePtr(&test_tex, -1, &size);
    for (u32 i = 0; i < size; i++)
    {
        int x = (i & 1) | ((i >> 1) & 2) | ((i >> 2) & 4) | ((i >> 3) & 0x1f8);
        int y = (((i >> 1) & 1) | ((i >> 2) & 2) | ((i >> 3) & 4) | ((i >> 9) & 0xf8));
        int col = x >> 7;
        x &= 0x7f;
        int tv = y % 8;
        int tu = x % 8;
        int tile = (y / 8) * 16 + (x / 8);
        auto palcol = ((*chr)[tile * 16 + tv] >> (7 - tu)) & 1;
        palcol |= (((*chr)[tile * 16 + 8 + tv] >> (7 - tu)) & 1) << 1;
        texdata[i] = (palcol == col) ? 0xff : 0;
    }

    C3D_TexSetFilter(&test_tex, GPU_NEAREST, GPU_NEAREST);
    C3D_TexBind(0, &test_tex);

    // Configure the first fragment shading substage to just pass through the vertex color
    // See https://www.opengl.org/sdk/docs/man2/xhtml/glTexEnv.xml for more insight
    C3D_TexEnv *env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_RGB, GPU_PRIMARY_COLOR);
    C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

    C3D_AlphaTest(true, GPU_NOTEQUAL, 0);
}

void CtrPpu::render()
{
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C3D_RenderTargetClear(target, C3D_CLEAR_ALL, palette[palettes[0]].rgb, 0);
    C3D_FrameDrawOn(target);
    C3D_SetViewport(0, 200 - 128, 240, 256);

    // Update the uniforms
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);

    C3D_ImmDrawBegin(GPU_TRIANGLES);

    float yoff = background_pattern_mode / 2.0f;
    if (true)
        for (int ty = 0; ty < 30; ty++)
        {
            for (int tx = 0; tx < 32; tx++)
            {
                auto tile = nametables[0][(29 - ty) * 32 + tx];
                auto metaattr = nametables[0][0x3c0 + ((29 - ty) >> 2) * 8 + (tx >> 2)];
                auto attr = (metaattr >> ((((29 - ty) & 2) | ((tx >> 1) & 1)) << 1)) & 3;
                for (int i = 1; i < 4; i++)
                {
                    float xo = i / 4.0f;
                    C3D_ImmSendAttrib(tx * 8, ty * 8, palette[palettes[attr * 4 + i]].r, palette[palettes[attr * 4 + i]].g);
                    C3D_ImmSendAttrib((tile & 0xf) / 64.0f + xo, 1 - (((tile >> 4) + 1) / 32.0f + yoff), palette[palettes[attr * 4 + i]].b, 0);

                    C3D_ImmSendAttrib(tx * 8 + 8, ty * 8, palette[palettes[attr * 4 + i]].r, palette[palettes[attr * 4 + i]].g);
                    C3D_ImmSendAttrib(((tile & 0xf) + 1) / 64.0f + xo, 1 - (((tile >> 4) + 1) / 32.0f + yoff), palette[palettes[attr * 4 + i]].b, 0);

                    C3D_ImmSendAttrib(tx * 8, ty * 8 + 8, palette[palettes[attr * 4 + i]].r, palette[palettes[attr * 4 + i]].g);
                    C3D_ImmSendAttrib((tile & 0xf) / 64.0f + xo, 1 - (((tile >> 4)) / 32.0f + yoff), palette[palettes[attr * 4 + i]].b, 0);

                    C3D_ImmSendAttrib(tx * 8, ty * 8 + 8, palette[palettes[attr * 4 + i]].r, palette[palettes[attr * 4 + i]].g);
                    C3D_ImmSendAttrib((tile & 0xf) / 64.0f + xo, 1 - (((tile >> 4)) / 32.0f + yoff), palette[palettes[attr * 4 + i]].b, 0);

                    C3D_ImmSendAttrib(tx * 8 + 8, ty * 8, palette[palettes[attr * 4 + i]].r, palette[palettes[attr * 4 + i]].g);
                    C3D_ImmSendAttrib(((tile & 0xf) + 1) / 64.0f + xo, 1 - (((tile >> 4) + 1) / 32.0f + yoff), palette[palettes[attr * 4 + i]].b, 0);

                    C3D_ImmSendAttrib(tx * 8 + 8, ty * 8 + 8, palette[palettes[attr * 4 + i]].r, palette[palettes[attr * 4 + i]].g);
                    C3D_ImmSendAttrib(((tile & 0xf) + 1) / 64.0f + xo, 1 - (((tile >> 4)) / 32.0f + yoff), palette[palettes[attr * 4 + i]].b, 0);
                }
            }
        }
    C3D_ImmDrawEnd();

    C3D_FrameEnd(0);
}