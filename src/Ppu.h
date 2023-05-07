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
    Ppu(std::shared_ptr<std::array<uint8_t, 0x2000>> chr) : chr(std::move(chr)) {}
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
    void render(uint32_t *image);
private:
    uint8_t mem_read(uint16_t addr);
    void mem_write(uint16_t addr, uint8_t value);
    uint8_t nametable_base;
    bool access_increment;
    bool sprite_pattern_mode;
    bool background_pattern_mode;
    bool sprite_size;
    bool vblank_enabled;
    bool addr_hi = true;
    bool scroll_latch = true;
    uint16_t access_addr;
    bool vblank_flag = false;
    uint8_t oam_addr = 0;
    uint8_t read_buf;
    uint8_t leftover;
};


#endif //NESEMU_PPU_H
