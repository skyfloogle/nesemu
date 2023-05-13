//
// Created by Floogle on 06/05/2023.
//

#ifndef NESEMU_CPU_H
#define NESEMU_CPU_H

#include <cstdint>
#include <array>
#include <memory>

#include "Ppu.h"

#define FLAG_V 1
#define FLAG_D 2
#define FLAG_I 4
#define FLAG_C 8
#define FLAG_Z 16
#define FLAG_N 32

class Cpu
{
public:
    Cpu(std::shared_ptr<std::array<uint8_t, 0x8000>> prg, std::unique_ptr<Ppu> ppu) : prg(std::move(prg)), ppu(std::move(ppu)) {}
    int run_instruction();
    void reset();
    void nmi();
    void vblank();
    bool just_wrote_oamdma;

private:
    uint8_t ram[0x800];
    uint8_t reg_a, reg_x, reg_y, reg_sp;
    bool flag_v, flag_d, flag_i, flag_c, flag_z, flag_n;
    uint16_t reg_pc = 0x8000;
    std::shared_ptr<std::array<uint8_t, 0x8000>> prg;
    std::unique_ptr<Ppu> ppu;

    bool reloading_controllers;
    uint8_t inputs[2];
    uint8_t input_mask[2] = {0};

    void perform_adc(uint8_t op);

    uint8_t mem_read(uint16_t addr);
    uint16_t mem_read16(uint16_t addr);
    void mem_write(uint16_t addr, uint8_t value);

    uint8_t add(uint8_t left, uint8_t right);
    uint8_t subtract(uint8_t left, uint8_t right);

    void perform_rol(uint8_t &value);

    void perform_ror(uint8_t &value);

    void perform_asl(uint8_t &value);

    void perform_lsr(uint8_t &value);
    void perform_bit(uint8_t value);

    uint16_t read_op16();
    uint16_t get_indx(uint8_t addr);
    uint16_t get_indy(uint8_t addr);
    void perform_php();
    void perform_plp();
    uint8_t pull();
    void push(uint8_t value);

    void perform_sbc(uint8_t op);
};

#endif // NESEMU_CPU_H
