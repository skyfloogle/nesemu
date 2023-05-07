//
// Created by Floogle on 07/05/2023.
//

#include "Ppu.h"
#include "palette.h"

uint8_t Ppu::read_reg(uint16_t addr) {
    uint8_t value = leftover;
    switch (addr & 7) {
        case 2:
            // PPUSTATUS
            value = vblank_flag << 7;
            vblank_flag = false;
            addr_hi = true;
            scroll_latch = true;
            break;
        case 4:
            // OAMDATA
            abort();
        case 7:
            // PPUDATA
            value = mem_read(access_addr);
            if (access_increment) access_addr += 32;
            else access_addr += 1;
            break;
        default:
            // not allowed
            break;
    }
    leftover = value;
    return value;
}

void Ppu::write_reg(uint16_t addr, uint8_t value) {
    leftover = value;
    switch (addr & 7) {
        case 0:
            // PPUCTRL
            nametable_base = value & 3;
            access_increment = (value & 4) != 0;
            sprite_pattern_mode = (value & 8) != 0;
            background_pattern_mode = (value & 16) != 0;
            sprite_size = (value & 32) != 0;
            vblank_enabled = (value & 128) != 0;
            break;
        case 1:
            // PPUMASK
            break;
        case 2:
            // PPUSTATUS, no write allowed
            abort();
        case 3:
            // OAMADDR
            oam_addr = value;
            break;
        case 4:
            // OAMDATA
            abort();
        case 5:
            // PPUSCROLL
            if (scroll_latch) scroll_x = value;
            else scroll_y = value;
            scroll_latch = !scroll_latch;
            break;
        case 6:
            // PPUADDR
            if (addr_hi) {
                access_addr = (access_addr & 0xff) | (value << 8);
            } else {
                access_addr = (access_addr & 0xff00) | value;
            }
            addr_hi = !addr_hi;
            break;
        case 7:
            // PPUDATA
            mem_write(access_addr, value);
            if (access_increment) access_addr += 32;
            else access_addr += 1;
            break;
    }
}

uint8_t Ppu::mem_read(uint16_t addr) {
    addr &= 0x3fff;
    uint8_t value = read_buf;
    if (addr < 0x2000) {
        read_buf = (*chr)[addr];
    } else if (addr < 0x3f00) {
        read_buf = nametables[0][addr & 0x3ff];
    } else {
        // palettes return early
        return palettes[addr & 0x1f];
    }
    return value;
}

void Ppu::mem_write(uint16_t addr, uint8_t value) {
    addr &= 0x3fff;
    if (addr < 0x2000) {
        // uhh rom?
        abort();
    } else if (addr < 0x3f00) {
        nametables[0][addr & 0x3ff] = value;
    } else {
        palettes[addr & 0x1f] = value;
    }
}

bool Ppu::vblank() {
    vblank_flag = true;
    return vblank_enabled;
}

void Ppu::write_oam(uint8_t value) {
    oam[oam_addr++] = value;
}

void Ppu::render(uint32_t* image) {
    uint32_t* image_cursor = image;
    uint8_t *tile_patterns = background_pattern_mode ? &(*chr)[0x1000] : &(*chr)[0];
    uint8_t *sprite_patterns = sprite_pattern_mode ? &(*chr)[0x1000] : &(*chr)[0];
    for (int y = 0; y < 240; y++) {
        int ty = y >> 3;
        int tv = y & 7;
        for (int x = 0; x < 256; x++) {
            int tx = x >> 3;
            int tu = x & 7;
            auto tile = nametables[0][ty * 32 + tx];
            auto palcol = (tile_patterns[tile * 16 + tv] >> (7 - tu)) & 1;
            palcol |= ((tile_patterns[tile * 16 + 8 + tv] >> (7 - tu)) & 1) << 1;
            if (palcol == 0) {
                *image_cursor++ = 0xff000000 | palette[palettes[0]];
            } else {
                auto metaattr = nametables[0][0x3c0 + (ty >> 2) * 8 + (tx >> 2)];
                auto attr = (metaattr >> (((ty >> 1) & 2 | (tx >> 1) & 1) << 1)) & 3;
                *image_cursor++ = 0xff000000 | palette[palettes[attr * 4 + palcol]];
            }
        }
    }
    // sprites
    for (int i = 0; i < 0x40; i++) {
        auto sy = oam[i * 4] + 1;
        auto tile = oam[i * 4 + 1];
        auto attrs = oam[i * 4 + 2];
        auto sx = oam[i * 4 + 3];
        auto pal = 4 + (attrs & 3);
        bool vflip = (attrs & 128) != 0;
        bool hflip = (attrs & 64) != 0;
        for (int sv_ = 0; sv_ < 8; sv_++) {
            int sv = vflip ? 7 - sv_ : sv_;
            int y = sy + sv_;
            if (y >= 240) break;
            for (int su_ = 0; su_ < 8; su_++) {
                int su = hflip ? 7 - su_ : su_;
                int x = sx + su_;
                if (x >= 256) break;
                auto palcol = (sprite_patterns[tile * 16 + sv] >> (7 - su)) & 1;
                palcol |= ((sprite_patterns[tile * 16 + 8 + sv] >> (7 - su)) & 1) << 1;
                if (palcol != 0) {
                    image[y * 256 + x] = 0xff000000 | palette[palettes[pal * 4 + palcol]];
                }
            }
        }
    }
}