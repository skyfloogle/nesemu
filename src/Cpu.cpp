//
// Created by Floogle on 06/05/2023.
//

#include "Cpu.h"
#include "input.h"

#include <string>

#define FLAGZ(val) flag_z = val == 0
#define FLAGN(val) flag_n = (val & 0x80) != 0
#define FLAGZN(val) FLAGZ(val); FLAGN(val)

void Cpu::reset() {
    reg_pc = mem_read16(0xfffc);
}

void Cpu::nmi() {
    push(reg_pc >> 8);
    push(reg_pc & 0xff);
    perform_php();
    reg_pc = mem_read16(0xfffa);
}

void Cpu::vblank() {
    if (ppu->vblank() && flag_i) nmi();
    ppu->render();
}

void Cpu::perform_rol(uint8_t& value) {
    bool new_c = (value & 0x80) != 0;
    value <<= 1;
    value |= uint8_t(flag_c);
    flag_c = new_c;
    FLAGZN(value);
}

void Cpu::perform_ror(uint8_t& value) {
    bool new_c = (value & 1) != 0;
    value >>= 1;
    value |= flag_c << 7;
    flag_c = new_c;
    FLAGZN(value);
}

void Cpu::perform_asl(uint8_t& value) {
    flag_c = (value & 0x80) != 0;
    value <<= 1;
    FLAGZN(value);
}

void Cpu::perform_lsr(uint8_t& value) {
    flag_c = (value & 1) != 0;
    value >>= 1;
    FLAGZN(value);
}

void Cpu::perform_bit(uint8_t value) {
    flag_n = (value & 0x80) != 0;
    flag_v = (value & 0x40) != 0;
    flag_z = (value & reg_a) == 0;
}

void Cpu::perform_adc(uint8_t op) {
    bool full_circle = op == 0xff && flag_c;
    reg_a = add(reg_a, op + flag_c);
    // special case, probably nicer way to do this
    flag_c = flag_c || full_circle;
}

uint8_t Cpu::add(uint8_t left, uint8_t right) {
    uint8_t result = left + right;
    FLAGZN(result);
    flag_c = (result < left);
    flag_v = (int16_t(int8_t(left)) + int16_t(int8_t(right)) != int16_t(int8_t(result)));
    return result;
}

void Cpu::perform_sbc(uint8_t op) {
    reg_a = subtract(reg_a, op + !flag_c);
}

uint8_t Cpu::subtract(uint8_t left, uint8_t right) {
    uint8_t result = left - right;
    FLAGZN(result);
    flag_c = left >= right;
    flag_v = (int16_t(int8_t(left)) - int16_t(int8_t(right)) != int16_t(int8_t(result)));
    return result;
}

int Cpu::run_instruction() {
    uint8_t tmp8;
    uint16_t tmp16;
    switch (mem_read(reg_pc++)) {
        case 0x01:
            // ORA x,ind
            reg_a |= mem_read(get_indx(mem_read(reg_pc++)));
            FLAGZN(reg_a);
            return 6;
        case 0x05:
            // ORA zpg
            reg_a |= ram[mem_read(reg_pc++)];
            FLAGZN(reg_a);
            return 3;
        case 0x06:
            // ASL zpg
            perform_asl(ram[mem_read(reg_pc++)]);
            return 5;
        case 0x08:
            // PHP
            perform_php();
            return 3;
        case 0x09:
            // ORA #
            reg_a |= mem_read(reg_pc++);
            FLAGZN(reg_a);
            return 2;
        case 0x0a:
            // ASL A
            perform_asl(reg_a);
            return 2;
        case 0x0d:
            // ORA abs
            reg_a |= mem_read(read_op16());
            FLAGZN(reg_a);
            return 4;
        case 0x0e:
            // ASL abs
            tmp16 = read_op16();
            tmp8 = mem_read(tmp16);
            perform_asl(tmp8);
            mem_write(tmp16, tmp8);
            return 6;
        case 0x10:
            // BPL
            tmp8 = mem_read(reg_pc++);
            if (!flag_n) {
                reg_pc += int8_t(tmp8);
                if ((reg_pc >> 8) != ((reg_pc - int8_t(tmp8)) >> 8))
                    return 4;
                else
                    return 3;
            }
            return 2;
        case 0x11:
            // ORA ind,Y
            tmp16 = get_indy(mem_read(reg_pc++));
            reg_a |= mem_read(tmp16);
            FLAGZN(reg_a);
            return 5 + ((tmp16 & 0xff) + 0x100 - reg_y >= 0x100);
        case 0x15:
            // ORA zpg,X
            reg_a |= ram[(mem_read(reg_pc++) + reg_x) & 0xff];
            FLAGZN(reg_a);
            return 4;
        case 0x16:
            // ASL zpg,X
            perform_asl(ram[(mem_read(reg_pc++) + reg_x) & 0xff]);
            return 6;
        case 0x18:
            // CLC
            flag_c = false;
            return 2;
        case 0x19:
            // ORA abs,Y
            tmp16 = read_op16();
            reg_a |= mem_read(tmp16 + reg_y);
            FLAGZN(reg_a);
            return 4 + ((tmp16 & 0xff) + reg_y < 0x100);
        case 0x1d:
            // ORA abs,X
            tmp16 = read_op16();
            reg_a |= mem_read(tmp16 + reg_x);
            FLAGZN(reg_a);
            return 4 + ((tmp16 & 0xff) + reg_x < 0x100);
        case 0x1e:
            // ASL abs,X
            tmp16 = read_op16() + reg_x;
            tmp8 = mem_read(tmp16);
            perform_asl(tmp8);
            mem_write(tmp16, tmp8);
            return 7;
        case 0x20:
            // JSR
            tmp16 = reg_pc + 1;
            push(tmp16 >> 8);
            push(tmp16 & 0xff);
            reg_pc = read_op16();
            return 6;
        case 0x21:
            // AND X,ind
            reg_a &= mem_read(get_indx(mem_read(reg_pc++)));
            FLAGZN(reg_a);
            return 6;
        case 0x24:
            // BIT zpg
            perform_bit(ram[mem_read(reg_pc++)]);
            return 3;
        case 0x25:
            // AND zpg
            reg_a &= ram[mem_read(reg_pc++)];
            FLAGZN(reg_a);
            return 3;
        case 0x26:
            // ROL zpg
            perform_rol(ram[mem_read(reg_pc++)]);
            return 5;
        case 0x28:
            // PLP
            perform_plp();
            return 4;
        case 0x29:
            // AND #
            reg_a &= mem_read(reg_pc++);
            FLAGZN(reg_a);
            return 2;
        case 0x2a:
            // ROL A
            perform_rol(reg_a);
            return 2;
        case 0x2c:
            // BIT abs
            perform_bit(mem_read(read_op16()));
            return 4;
        case 0x2d:
            // AND abs
            reg_a &= mem_read(read_op16());
            FLAGZN(reg_a);
            return 4;
        case 0x2e:
            // ROL abs
            tmp16 = read_op16();
            tmp8 = mem_read(tmp16);
            perform_rol(tmp8);
            mem_write(tmp16, tmp8);
            return 6;
        case 0x30:
            // BMI
            tmp8 = mem_read(reg_pc++);
            if (flag_n) {
                reg_pc += int8_t(tmp8);
                if ((reg_pc >> 8) != ((reg_pc - int8_t(tmp8)) >> 8))
                    return 4;
                else
                    return 3;
            }
            return 2;
        case 0x31:
            // AND ind,Y
            tmp16 = get_indy(mem_read(reg_pc++));
            reg_a &= mem_read(tmp16);
            FLAGZN(reg_a);
            return 5 + ((tmp16 & 0xff) + 0x100 - reg_y >= 0x100);
        case 0x35:
            // AND zpg,X
            reg_a &= ram[(mem_read(reg_pc++) + reg_x) & 0xff];
            FLAGZN(reg_a);
            return 4;
        case 0x36:
            // ROL zpg,X
            tmp16 = (mem_read(reg_pc++) + reg_x) & 0xff;
            perform_rol(ram[tmp16]);
            return 6;
        case 0x38:
            // SEC
            flag_c = true;
            return 2;
        case 0x39:
            // AND abs,Y
            tmp16 = read_op16();
            reg_a &= tmp16 + reg_y;
            FLAGZN(reg_a);
            return 4 + (tmp16 >> 8 != ((tmp16 + reg_y) >> 8));
        case 0x3d:
            // AND abs,X
            tmp16 = read_op16();
            reg_a &= tmp16 + reg_x;
            FLAGZN(reg_a);
            return 4 + (tmp16 >> 8 != ((tmp16 + reg_x) >> 8));
        case 0x3e:
            // ROL abs,X
            tmp16 = read_op16() + reg_x;
            tmp8 = mem_read(tmp16);
            perform_rol(tmp8);
            mem_write(tmp16, tmp8);
            return 7;
        case 0x40:
            // RTI
            perform_plp();
            reg_pc = pull();
            reg_pc |= pull() << 8;
            return 6;
        case 0x41:
            // EOR X,ind
            reg_a ^= mem_read(get_indx(mem_read(reg_pc++)));
            FLAGZN(reg_a);
            return 6;
        case 0x45:
            // EOR zpg
            reg_a ^= ram[mem_read(reg_pc++)];
            FLAGZN(reg_a);
            return 3;
        case 0x46:
            // LSR zpg
            perform_lsr(ram[mem_read(reg_pc++)]);
            return 5;
        case 0x48:
            // PHA
            push(reg_a);
            return 3;
        case 0x49:
            // EOR #
            reg_a ^= mem_read(reg_pc++);
            FLAGZN(reg_a);
            return 2;
        case 0x4a:
            // LSR A
            perform_lsr(reg_a);
            return 2;
        case 0x4c:
            // JMP abs
            reg_pc = read_op16();
            return 3;
        case 0x4d:
            // EOR abs
            reg_a ^= mem_read(read_op16());
            FLAGZN(reg_a);
            return 4;
        case 0x4e:
            // LSR abs
            tmp16 = read_op16();
            tmp8 = mem_read(tmp16);
            perform_lsr(tmp8);
            mem_write(tmp16, tmp8);
            return 6;
        case 0x50:
            // BVC
            tmp8 = mem_read(reg_pc++);
            if (!flag_v) {
                reg_pc += int8_t(tmp8);
                if ((reg_pc >> 8) != ((reg_pc - int8_t(tmp8)) >> 8))
                    return 4;
                else
                    return 3;
            }
            return 2;
        case 0x51:
            // EOR ind,Y
            tmp16 = get_indy(mem_read(reg_pc++));
            reg_a ^= mem_read(tmp16);
            FLAGZN(reg_a);
            return 5 + ((tmp16 & 0xff) + 0x100 - reg_y >= 0x100);
        case 0x55:
            // EOR zpg,X
            reg_a ^= ram[(mem_read(reg_pc++) + reg_x) & 0xff];
            FLAGZN(reg_a);
            return 4;
        case 0x56:
            // LSR zpg,X
            perform_lsr(ram[(mem_read(reg_pc++) + reg_x) & 0xff]);
            return 6;
        case 0x58:
            // CLI
            flag_i = false;
            return 2;
        case 0x59:
            // EOR abs,Y
            tmp16 = read_op16();
            reg_a ^= mem_read(tmp16 + reg_y);
            FLAGZN(reg_a);
            return 4 + (tmp16 >> 8 != ((tmp16 + reg_y) >> 8));
        case 0x5d:
            // EOR abs,X
            tmp16 = read_op16();
            reg_a ^= mem_read(tmp16 + reg_x);
            FLAGZN(reg_a);
            return 4 + (tmp16 >> 8 != ((tmp16 + reg_x) >> 8));
        case 0x5e:
            // LSR abs,X
            tmp16 = read_op16() + reg_x;
            tmp8 = mem_read(tmp16);
            perform_lsr(tmp8);
            mem_write(tmp16, tmp8);
            return 7;
        case 0x60:
            // RTS
            reg_pc = pull();
            reg_pc |= pull() << 8;
            reg_pc++;
            return 6;
        case 0x61:
            // ADC X,ind
            perform_adc(mem_read(get_indx(mem_read(reg_pc++))));
            return 6;
        case 0x65:
            // ADC zpg
            perform_adc(ram[mem_read(reg_pc++)]);
            return 3;
        case 0x66:
            // ROR zpg
            perform_ror(ram[mem_read(reg_pc++)]);
            return 5;
        case 0x68:
            // PLA
            reg_a = pull();
            FLAGZN(reg_a);
            return 4;
        case 0x69:
            // ADC #
            perform_adc(mem_read(reg_pc++));
            return 2;
        case 0x6a:
            // ROR A
            perform_ror(reg_a);
            return 2;
        case 0x6c:
            // JMP ind
            reg_pc = mem_read16(read_op16());
            return 5;
        case 0x6d:
            // ADC abs
            perform_adc(mem_read(read_op16()));
            return 4;
        case 0x6e:
            // ROR abs
            tmp16 = read_op16();
            tmp8 = mem_read(tmp16);
            perform_ror(tmp8);
            mem_write(tmp16, tmp8);
            return 6;
        case 0x70:
            // BVS
            tmp8 = mem_read(reg_pc++);
            if (flag_v) {
                reg_pc += int8_t(tmp8);
                if ((reg_pc >> 8) != ((reg_pc - int8_t(tmp8)) >> 8))
                    return 4;
                else
                    return 3;
            }
            return 2;
        case 0x71:
            // ADC ind,Y
            tmp16 = get_indy(mem_read(reg_pc++));
            perform_adc(mem_read(tmp16));
            return 5 + ((tmp16 & 0xff) + 0x100 - reg_y >= 0x100);
        case 0x75:
            // ADC zpg,X
            perform_adc(ram[(mem_read(reg_pc++) + reg_x) & 0xff]);
            return 4;
        case 0x76:
            // ROR zpg,X
            perform_ror(ram[(mem_read(reg_pc++) + reg_x) & 0xff]);
            return 6;
        case 0x78:
            // SEI
            flag_i = true;
            return 2;
        case 0x79:
            // ADC abs,Y
            tmp16 = read_op16();
            perform_adc(mem_read(tmp16 + reg_y));
            return 4 + ((tmp16 & 0xff) + reg_y < 0x100);
        case 0x7d:
            // ADC abs,X
            tmp16 = read_op16();
            perform_adc(mem_read(tmp16 + reg_x));
            return 4 + ((tmp16 & 0xff) + reg_x < 0x100);
        case 0x7e:
            // ROR abs,X
            tmp16 = read_op16() + reg_x;
            tmp8 = mem_read(tmp16);
            perform_ror(tmp8);
            mem_write(tmp16, tmp8);
            return 7;
        case 0x81:
            // STA X,ind
            mem_write(get_indx(mem_read(reg_pc++)), reg_a);
            return 6;
        case 0x84:
            // STY zpg
            ram[mem_read(reg_pc++)] = reg_y;
            return 3;
        case 0x85:
            // STA zpg
            ram[mem_read(reg_pc++)] = reg_a;
            return 3;
        case 0x86:
            // STX zpg
            ram[mem_read(reg_pc++)] = reg_x;
            return 3;
        case 0x88:
            // DEY
            reg_y--;
            FLAGZN(reg_y);
            return 2;
        case 0x8a:
            // TXA
            reg_a = reg_x;
            FLAGZN(reg_a);
            return 2;
        case 0x8c:
            // STY abs
            mem_write(read_op16(), reg_y);
            return 4;
        case 0x8d:
            // STA abs
            mem_write(read_op16(), reg_a);
            return 4;
        case 0x8e:
            // STX abs
            mem_write(read_op16(), reg_x);
            return 4;
        case 0x90:
            // BCC
            tmp8 = mem_read(reg_pc++);
            if (!flag_c) {
                reg_pc += int8_t(tmp8);
                if ((reg_pc >> 8) != ((reg_pc - int8_t(tmp8)) >> 8))
                    return 4;
                else
                    return 3;
            }
            return 2;
        case 0x91:
            // STA ind,Y
            mem_write(get_indy(mem_read(reg_pc++)), reg_a);
            return 6;
        case 0x94:
            // STY zpg,X
            ram[(mem_read(reg_pc++) + reg_x) & 0xff] = reg_y;
            return 4;
        case 0x95:
            // STA zpg,X
            ram[(mem_read(reg_pc++) + reg_x) & 0xff] = reg_a;
            return 4;
        case 0x96:
            // STX zpg,Y
            ram[(mem_read(reg_pc++) + reg_y) & 0xff] = reg_x;
            return 4;
        case 0x98:
            // TYA
            reg_a = reg_y;
            FLAGZN(reg_a);
            return 2;
        case 0x99:
            // STA abs,Y
            mem_write(read_op16() + reg_y, reg_a);
            return 5;
        case 0x9a:
            // TXS
            reg_sp = reg_x;
            return 2;
        case 0x9d:
            // STA abs,X
            mem_write(read_op16() + reg_x, reg_a);
            return 5;
        case 0xa0:
            // LDY #
            reg_y = mem_read(reg_pc++);
            FLAGZN(reg_y);
            return 2;
        case 0xa1:
            // LDA X,ind
            reg_a = mem_read(get_indx(mem_read(reg_pc++)));
            FLAGZN(reg_a);
            return 6;
        case 0xa2:
            // LDX #
            reg_x = mem_read(reg_pc++);
            FLAGZN(reg_x);
            return 2;
        case 0xa4:
            // LDY zpg
            reg_y = ram[mem_read(reg_pc++)];
            FLAGZN(reg_y);
            return 3;
        case 0xa5:
            // LDA zpg
            reg_a = ram[mem_read(reg_pc++)];
            FLAGZN(reg_a);
            return 3;
        case 0xa6:
            // LDX zpg
            reg_x = ram[mem_read(reg_pc++)];
            FLAGZN(reg_x);
            return 3;
        case 0xa8:
            // TAY
            reg_y = reg_a;
            FLAGZN(reg_y);
            return 2;
        case 0xa9:
            // LDA #
            reg_a = mem_read(reg_pc++);
            FLAGZN(reg_a);
            return 2;
        case 0xaa:
            // TAX
            reg_x = reg_a;
            FLAGZN(reg_x);
            return 2;
        case 0xac:
            // LDY abs
            reg_y = mem_read(read_op16());
            FLAGZN(reg_y);
            return 4;
        case 0xad:
            // LDA abs
            reg_a = mem_read(read_op16());
            FLAGZN(reg_a);
            return 4;
        case 0xae:
            // LDX abs
            reg_x = mem_read(read_op16());
            FLAGZN(reg_x);
            return 4;
        case 0xb0:
            // BCS
            tmp8 = mem_read(reg_pc++);
            if (flag_c) {
                reg_pc += int8_t(tmp8);
                if ((reg_pc >> 8) != ((reg_pc - int8_t(tmp8)) >> 8))
                    return 4;
                else
                    return 3;
            }
            return 2;
        case 0xb1:
            // LDA ind,Y
            tmp8 = mem_read(reg_pc++);
            tmp16 = get_indy(tmp8);
            reg_a = mem_read(tmp16);
            FLAGZN(reg_a);
            return 5 + ((tmp16 & 0xff) + 0x100 - reg_y >= 0x100);
        case 0xb4:
            // LDY zpg,X
            reg_y = ram[(mem_read(reg_pc++) + reg_x) & 0xff];
            FLAGZN(reg_y);
            return 4;
        case 0xb5:
            // LDA zpg,X
            reg_a = ram[(mem_read(reg_pc++) + reg_x) & 0xff];
            FLAGZN(reg_a);
            return 4;
        case 0xb6:
            // LDX zpg,Y
            reg_x = ram[(mem_read(reg_pc++) + reg_y) & 0xff];
            FLAGZN(reg_x);
            return 4;
        case 0xb8:
            // CLV
            flag_v = false;
            return 2;
        case 0xb9:
            // LDA abs,Y
            tmp16 = read_op16();
            reg_a = mem_read(tmp16 + reg_y);
            FLAGZN(reg_a);
            return 4 + (tmp16 >> 8 != ((tmp16 + reg_y) >> 8));
        case 0xba:
            // TSX
            reg_x = reg_sp;
            FLAGZN(reg_x);
            return 2;
        case 0xbc:
            // LDY abs,X
            tmp16 = read_op16();
            reg_y = mem_read(tmp16 + reg_x);
            FLAGZN(reg_y);
            return 4 + (tmp16 >> 8 != ((tmp16 + reg_x) >> 8));
        case 0xbd:
            // LDA abs,X
            tmp16 = read_op16();
            reg_a = mem_read(tmp16 + reg_x);
            FLAGZN(reg_a);
            return 4 + (tmp16 >> 8 != ((tmp16 + reg_x) >> 8));
        case 0xbe:
            // LDX abs,Y
            tmp16 = read_op16();
            reg_x = mem_read(tmp16 + reg_y);
            FLAGZN(reg_x);
            return 4 + (tmp16 >> 8 != ((tmp16 + reg_y) >> 8));
        case 0xc0:
            // CPY #
            subtract(reg_y, mem_read(reg_pc++));
            return 2;
        case 0xc1:
            // CMP X,ind
            subtract(reg_a, mem_read(get_indx(mem_read(reg_pc++))));
            return 6;
        case 0xc4:
            // CPY zpg
            subtract(reg_y, ram[mem_read(reg_pc++)]);
            return 3;
        case 0xc5:
            // CMP zpg
            subtract(reg_a, ram[mem_read(reg_pc++)]);
            return 3;
        case 0xc6:
            // DEC zpg
            tmp8 = mem_read(reg_pc++);
            ram[tmp8]--;
            FLAGZN(ram[tmp8]);
            return 5;
        case 0xc8:
            // INY
            reg_y++;
            FLAGZN(reg_y);
            return 2;
        case 0xc9:
            // CMP #
            subtract(reg_a, mem_read(reg_pc++));
            return 2;
        case 0xca:
            // DEX
            reg_x--;
            FLAGZN(reg_x);
            return 2;
        case 0xcc:
            // CPY abs
            subtract(reg_y, mem_read(read_op16()));
            return 4;
        case 0xcd:
            // CMP abs
            subtract(reg_a, mem_read(read_op16()));
            return 4;
        case 0xce:
            // DEC abs
            tmp16 = read_op16();
            tmp8 = mem_read(tmp16);
            mem_write(tmp16, --tmp8);
            FLAGZN(tmp8);
            return 6;
        case 0xd0:
            // BNE
            tmp8 = mem_read(reg_pc++);
            if (!flag_z) {
                reg_pc += int8_t(tmp8);
                if ((reg_pc >> 8) != ((reg_pc - int8_t(tmp8)) >> 8))
                    return 4;
                else
                    return 3;
            }
            return 2;
        case 0xd1:
            // CMP ind,Y
            tmp16 = get_indy(mem_read(reg_pc++));
            subtract(reg_a, mem_read(tmp16));
            return 5 + ((tmp16 & 0xff) + 0x100 - reg_y >= 0x100);
        case 0xd5:
            // CMP zpg,X
            subtract(reg_a, ram[(mem_read(reg_pc++) + reg_x) & 0xff]);
            return 4;
        case 0xd6:
            // DEC zpg,X
            tmp8 = mem_read(reg_pc++) + reg_x;
            ram[tmp8]--;
            FLAGZN(ram[tmp8]);
            return 6;
        case 0xd8:
            // CLD
            flag_d = false;
            return 2;
        case 0xd9:
            // CMP abs,Y
            tmp16 = read_op16();
            subtract(reg_a, mem_read(tmp16 + reg_y));
            return 4 + (tmp16 >> 8 != ((tmp16 + reg_y) >> 8));
        case 0xdd:
            // CMP abs,X
            tmp16 = read_op16();
            subtract(reg_a, mem_read(tmp16 + reg_x));
            return 4 + (tmp16 >> 8 != ((tmp16 + reg_x) >> 8));
        case 0xde:
            // DEC abs,X
            tmp16 = read_op16() + reg_x;
            tmp8 = mem_read(tmp16);
            mem_write(tmp16, --tmp8);
            FLAGZN(tmp8);
            return 7;
        case 0xe0:
            // CPX #
            subtract(reg_x, mem_read(reg_pc++));
            return 2;
        case 0xe1:
            // SBC X,ind
            perform_sbc(mem_read(get_indx(mem_read(reg_pc++))));
            return 6;
        case 0xe4:
            // CPX zpg
            subtract(reg_x, ram[mem_read(reg_pc++)]);
            return 3;
        case 0xe5:
            // SBC zpg
            perform_sbc(ram[mem_read(reg_pc++)]);
            return 3;
        case 0xe6:
            // INC zpg
            tmp8 = mem_read(reg_pc++);
            ram[tmp8]++;
            FLAGZN(ram[tmp8]);
            return 5;
        case 0xe8:
            // INX
            reg_x++;
            FLAGZN(reg_x);
            return 2;
        case 0xe9:
            // SBC #
            perform_sbc(mem_read(reg_pc++));
            return 2;
        case 0xea:
            // NOP
            return 2;
        case 0xec:
            // CPX abs
            subtract(reg_x, mem_read(read_op16()));
            return 4;
        case 0xed:
            // SBC abs
            perform_sbc(mem_read(read_op16()));
            return 4;
        case 0xee:
            // INC abs
            tmp16 = read_op16();
            tmp8 = mem_read(tmp16);
            mem_write(tmp16, ++tmp8);
            FLAGZN(tmp8);
            return 6;
        case 0xf0:
            // BEQ
            tmp8 = mem_read(reg_pc++);
            if (flag_z) {
                reg_pc += int8_t(tmp8);
                if ((reg_pc >> 8) != ((reg_pc - int8_t(tmp8)) >> 8))
                    return 4;
                else
                    return 3;
            }
            return 2;
        case 0xf1:
            // SBC ind,Y
            tmp16 = get_indy(mem_read(reg_pc++));
            perform_sbc(mem_read(tmp16));
            return 5 + ((tmp16 & 0xff) + 0x100 - reg_y >= 0x100);
        case 0xf5:
            // SBC zpg,X
            perform_sbc(ram[(mem_read(reg_pc++) + reg_x) & 0xff]);
            return 4;
        case 0xf6:
            // INC zpg,X
            tmp8 = mem_read(reg_pc++) + reg_x;
            ram[tmp8]++;
            FLAGZN(tmp8);
            return 6;
        case 0xf8:
            // SED
            flag_d = true;
            return 2;
        case 0xf9:
            // SBC abs,Y
            tmp16 = read_op16();
            perform_sbc(mem_read(tmp16 + reg_y));
            return 4 + (tmp16 >> 8 != ((tmp16 + reg_y) >> 8));
        case 0xfd:
            // SBC abs,X
            tmp16 = read_op16();
            perform_sbc(mem_read(tmp16 + reg_x));
            return 4 + (tmp16 >> 8 != ((tmp16 + reg_x) >> 8));
        case 0xfe:
            // INC abs,X
            tmp16 = read_op16() + reg_x;
            tmp8 = mem_read(tmp16);
            mem_write(tmp16, ++tmp8);
            FLAGZN(tmp8);
            return 7;
        default:
            printf("unimplemented opcode %d", mem_read(reg_pc - 1));
            abort();
    }
}

uint8_t Cpu::mem_read(uint16_t addr) {
    if (addr >= 0x8000) {
        return (*prg)[addr & 0x7fff];
    } else if (addr < 0x2000) {
        return ram[addr & 0x7ff];
    } else if (addr < 0x4000) {
        return ppu->read_reg(addr);
    } else if (addr == 0x4016) {
        // JOY1
        input_mask[0] <<= 1;
        if (input_mask[0] == 0) input_mask[0] = 1;
        return (inputs[0] & input_mask[0]) != 0;
    } else if (addr == 0x4017) {
        // JOY2
        input_mask[1] <<= 1;
        if (input_mask[1] == 0) input_mask[1] = 1;
        return inputs[1] & input_mask[1];
    } else {
        // TODO
        abort();
    }
}

void Cpu::mem_write(uint16_t addr, uint8_t value) {
    if (addr < 0x2000) {
        ram[addr & 0x7ff] = value;
    } else if (addr < 0x4000) {
        ppu->write_reg(addr, value);
    } else if (addr < 0x4014 || addr == 0x4015) {
        // audio
        return;
    } else if (addr == 0x4014) {
        for (int i = 0; i < 256; i++) {
            ppu->write_oam(mem_read((value << 8) | i));
        }
        just_wrote_oamdma = true;
    } else if (addr == 0x4016) {
        // JOY1
        reloading_controllers = (value & 1) != 0;
        if (reloading_controllers) {
            inputs[0] = read_inputs();
        }
    } else if (addr == 0x4017) {
        // "frame counter"
    } else {
        // TODO
        abort();
    }
}

uint16_t Cpu::mem_read16(uint16_t addr) {
    uint16_t out = mem_read(addr);
    out |= mem_read(addr + 1) << 8;
    return out;
}

uint16_t Cpu::read_op16() {
    uint16_t op = mem_read(reg_pc++);
    op |= mem_read(reg_pc++) << 8;
    return op;
}

uint16_t Cpu::get_indx(uint8_t addr) {
    uint16_t out = ram[(addr + reg_x) & 0xff];
    out |= ram[(addr + reg_x + 1) & 0xff] << 8;
    return out;
}

uint16_t Cpu::get_indy(uint8_t addr) {
    uint16_t out = ram[(addr) & 0xff];
    out |= ram[(addr + 1) & 0xff] << 8;
    return out + reg_y;
}

void Cpu::push(uint8_t value) {
    ram[0x100 + reg_sp--] = value;
}

uint8_t Cpu::pull() {
    return ram[0x100 + ++reg_sp];
}

void Cpu::perform_php() {
    push(
            (flag_c << 0) |
            (flag_z << 1) |
            (flag_i << 2) |
            (flag_d << 3) |
            (flag_v << 6) |
            (flag_n << 7));
}

void Cpu::perform_plp() {
    uint8_t reg = pull();
    flag_c = (reg & 1) != 0;
    flag_z = (reg & 2) != 0;
    flag_i = (reg & 4) != 0;
    flag_d = (reg & 8) != 0;
    flag_v = (reg & 64) != 0;
    flag_n = (reg & 128) != 0;
}