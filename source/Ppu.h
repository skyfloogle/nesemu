//
// Created by Floogle on 07/05/2023.
//

#ifndef NESEMU_PPU_H
#define NESEMU_PPU_H

#include <array>
#include <cstdint>
#include <memory>

class Ppu {
public:
    Ppu(std::shared_ptr<std::array<uint8_t, 0x2000>> chr, bool vertical) : chr(std::move(chr)), vertical_mirror(vertical) {}
    uint8_t read_reg(uint16_t addr);
    void write_reg(uint16_t addr, uint8_t value);
    bool vblank();
    void write_oam(uint8_t value);
    virtual void render() = 0;
protected:
    std::shared_ptr<std::array<uint8_t, 0x2000>> chr;
    std::array<std::array<uint8_t, 0x400>, 2> nametables;
    std::array<uint8_t, 0x20> palettes;
    std::array<uint8_t, 0x100> oam;
    uint8_t scroll_x;
    uint8_t scroll_y;
    bool vertical_mirror;
    void render(uint32_t *image);
private:
    uint8_t mem_read(uint16_t addr);
    void mem_write(uint16_t addr, uint8_t value);
    uint8_t nametable_base = 0;
    bool access_increment = false;
    bool sprite_pattern_mode = false;
    bool background_pattern_mode = false;
    bool sprite_size = false;
    bool vblank_enabled = false;
    bool addr_hi = true;
    bool scroll_latch = true;
    uint16_t access_addr = 0;
    bool vblank_flag = false;
    uint8_t oam_addr = 0;
    uint8_t read_buf;
    uint8_t leftover;
};


#endif //NESEMU_PPU_H
