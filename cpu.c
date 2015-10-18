#include <stdio.h>
#include <string.h>

#include "cpu.h"

void cpu_init(cpu_t *cpu, uint8_t const *rom, uint8_t *cart) {
    memset(cpu, 0, sizeof(*cpu));

    memcpy(cpu->rom, rom, 256);
    cpu->cart = cart;
    cpu->pc = 0;
    cpu->lcdc = 0x83;
    SET8(cpu, 0xff50, 0);
}

void dump(cpu_t const *cpu) {
    printf("\n");
    printf("======== CPU DUMP ========\n");
    printf(" A: %02x      F: %02x\n", cpu->a, cpu->f);
    printf(" B: %02x      C: %02x\n", cpu->b, cpu->c);
    printf(" D: %02x      E: %02x\n", cpu->d, cpu->e);
    printf(" H: %02x      L: %02x\n", cpu->h, cpu->l);
    printf("SP: %04x   PC: %04x\n", cpu->sp, cpu->pc);
    printf("\n");
    printf(" LCDC: %02x\n", cpu->lcdc);
    printf("SCX/Y: %02x/%02x\n", cpu->lcdc_scx, cpu->lcdc_scy);
    printf("==========================\n");
    printf("\n");
}

uint8_t GET8(cpu_t const *cpu, uint16_t addr) {
    if (addr < 0x100 && cpu->ram[0xff50] == 0x00) {
        return cpu->rom[addr];
    } else if (addr < 0x8000) {
        return cpu->cart[addr];
    } else if (addr == 0xff40) {
        // LCDC
        return cpu->lcdc;
    } else if (addr == 0xff41) {
        // STAT
        return cpu->lcdc_mode;
    } else if (addr == 0xff42) {
        // SCY
        return cpu->lcdc_scy;
    } else if (addr == 0xff43) {
        // SCX
        return cpu->lcdc_scx;
    } else if (addr == 0xff44) {
        // LY
        return cpu->lcdc_line;
    }
    return cpu->ram[addr];
}

void SET8(cpu_t *cpu, uint16_t addr, uint8_t v) {
    if (addr == 0xff50 && v != 0) {
        printf("DMG ROM overlay disabled\n");
    }
    if (addr == 0xff40) {
        // LCDC
        cpu->lcdc = v;
    } else if (addr == 0xff41) {
        // STAT
        // Low 3 bits are R/O.
        cpu->lcdc_mode = (cpu->lcdc_mode & 0x7) | (v & 0xf8);
    } else if (addr == 0xff42) {
        // SCY
        cpu->lcdc_scy = v;
    } else if (addr == 0xff43) {
        // SCX
        cpu->lcdc_scx = v;
    } else {
        cpu->ram[addr] = v;
    }
}

uint8_t REG8(cpu_t const *cpu, int s) {
    switch (s) {
        case 0x7: return cpu->a;
        case 0x0: return cpu->b;
        case 0x1: return cpu->c;
        case 0x2: return cpu->d;
        case 0x3: return cpu->e;
        case 0x4: return cpu->h;
        case 0x5: return cpu->l;
        case 0x6: return GET8(cpu, cpu->hl);
        default:
            fprintf(stderr, "REG8 got %x\n", s);
            exit(1);
    }
}

void SREG8(cpu_t *cpu, int s, uint8_t v) {
    switch (s) {
        case 0x7: cpu->a = v; break;
        case 0x0: cpu->b = v; break;
        case 0x1: cpu->c = v; break;
        case 0x2: cpu->d = v; break;
        case 0x3: cpu->e = v; break;
        case 0x4: cpu->h = v; break;
        case 0x5: cpu->l = v; break;
        case 0x6: SET8(cpu, cpu->hl, v); break;
        default:
            fprintf(stderr, "SREG8 got %x\n", s);
            exit(1);
    }
}

char const *REG8N(int s) {
    switch (s) {
        case 0x7: return "A";
        case 0x0: return "B";
        case 0x1: return "C";
        case 0x2: return "D";
        case 0x3: return "E";
        case 0x4: return "H";
        case 0x5: return "L";
        case 0x6: return "(HL)";
        default:
            fprintf(stderr, "REG8N got %x\n", s);
            exit(1);
    }
}

uint16_t REG16(cpu_t *cpu, int s) {
    switch (s) {
        case 0x0: return cpu->bc;
        case 0x1: return cpu->de;
        case 0x2: return cpu->hl;
        case 0x3: return cpu->sp;
        default:
            fprintf(stderr, "REG16 got %x\n", s);
            exit(1);
    }
}

void SREG16(cpu_t *cpu, int s, uint16_t v) {
    switch (s) {
        case 0x0: cpu->bc = v; break;
        case 0x1: cpu->de = v; break;
        case 0x2: cpu->hl = v; break;
        case 0x3: cpu->sp = v; break;
        default:
            fprintf(stderr, "SREG16 got %x\n", s);
            exit(1);
    }
}

char const *REG16N(int s) {
    switch (s) {
        case 0x0: return "BC";
        case 0x1: return "DE";
        case 0x2: return "HL";
        case 0x3: return "SP";
        default:
            fprintf(stderr, "REG16N got %x\n", s);
            exit(1);
    }
}

char const *CCN(int s) {
    switch (s) {
        case 0x0: return "NZ";
        case 0x1: return "Z";
        case 0x2: return "NC";
        case 0x3: return "C";
        default:
            fprintf(stderr, "CCN got %x\n", s);
            exit(1);
    }
}

void PUSH16(cpu_t *cpu, uint16_t v) {
    cpu->sp -= 2;
    SET8(cpu, cpu->sp, v & 0xff);
    SET8(cpu, cpu->sp + 1, (v >> 8) & 0xff);
}

uint16_t POP16(cpu_t *cpu) {
    uint16_t v = GET8(cpu, cpu->sp + 1);
    v = (v << 8) | GET8(cpu, cpu->sp);
    cpu->sp += 2;
    return v;
}

static int show_dis = 0;
#define DIS if (show_dis)

int step(cpu_t *cpu) {
    DIS { printf("%04x: ", cpu->pc); }

    uint8_t b = GET8(cpu, cpu->pc++);

    if ((b & 0xc0) == 0x40) {
        // LD r,r'
        uint8_t dst = (b >> 3) & 0x7,
                src = b & 0x7;
        DIS { printf("LD %s,%s\n", REG8N(dst), REG8N(src)); }
        SREG8(cpu, dst, REG8(cpu, src));

        // no flags set
        return dst == 0x6 || src == 0x6 ? 8 : 4;
    } else if ((b & 0xc7) == 0x06) {
        // LD r,n
        uint8_t r = (b >> 3) & 0x7;
        uint8_t v = GET8(cpu, cpu->pc++);
        DIS { printf("LD %s,$%x\n", REG8N(r), v); }
        SREG8(cpu, r, v);

        // no flags set
        return 8;
    } else if (b == 0x1a) {
        // LD A,(DE)
        DIS { printf("LD A,(DE)\n"); }
        cpu->a = GET8(cpu, cpu->de);

        // no flags set
        return 8;
    } else if (b == 0xe2) {
        // LD ($FF00+C),A
        DIS { printf("LD ($FF00+C),A\n"); }
        SET8(cpu, 0xff00 + cpu->c, cpu->a);

        // no flags set
        return 8;
    } else if (b == 0xf0) {
        // LD A,($FF00+n)
        uint8_t n = GET8(cpu, cpu->pc++);
        DIS { printf("LD A,($FF00+$%x)\n", n); }
        cpu->a = GET8(cpu, 0xff00 + n);

        // no flags set
        return 12;
    } else if (b == 0xe0) {
        // LD ($FF00+n),A
        uint8_t n = GET8(cpu, cpu->pc++);
        DIS { printf("LD ($FF00+$%x),A\n", n); }
        SET8(cpu, 0xff00 + n, cpu->a);

        // no flags set
        return 12;
    } else if (b == 0xea) {
        // LD (nn),A
        uint16_t v = GET8(cpu, cpu->pc++);
        v |= GET8(cpu, cpu->pc++) << 8;
        DIS { printf("LD ($%04x),A\n", v); }
        SET8(cpu, v, cpu->a);

        // no flags set
        return 16;
    } else if (b == 0x22) {
        // LD (HL+),A
        DIS { printf("LD (HL+),A\n"); }
        SET8(cpu, cpu->hl++, cpu->a);

        // no flags set
        return 8;
    } else if (b == 0x32) {
        // LD (HL-),A
        DIS { printf("LD (HL-),A\n"); }
        SET8(cpu, cpu->hl--, cpu->a);

        // no flags set
        return 8;
    } else if ((b & 0xcf) == 0x01) {
        // LD dd,nn
        uint8_t r = (b >> 4) & 0x3;
        uint16_t v = GET8(cpu, cpu->pc++);
        v |= GET8(cpu, cpu->pc++) << 8;
        DIS { printf("LD %s,$%04x\n", REG16N(r), v); }
        SREG16(cpu, r, v);

        // no flags set
        return 12;
    } else if ((b & 0xcf) == 0xc5) {
        // PUSH qq
        uint8_t qq = (b >> 4) & 0x3;
        DIS { printf("PUSH %s\n", REG16N(qq)); }
        PUSH16(cpu, REG16(cpu, qq));

        // no flags set
        return 16;
    } else if ((b & 0xcf) == 0xc1) {
        // POP qq
        uint8_t qq = (b >> 4) & 0x3;
        DIS { printf("POP %s\n", REG16N(qq)); }
        SREG16(cpu, qq, POP16(cpu));

        // no flags set
        return 12;
    } else if ((b & 0xf8) == 0x80) {
        // ADD A,r
        uint8_t r = b & 0x7;
        DIS { printf("ADD A,%s\n", REG8N(r)); }
        uint8_t v = REG8(cpu, r);
        cpu->fh = (((cpu->a & 0xf) + (v & 0xf)) & 0x10) == 0x10;
        cpu->fc = ((uint16_t) cpu->a) + ((uint16_t) v) > 0xff;
        cpu->a += v;
        cpu->fz = cpu->a == 0;
        cpu->fn = 0;
        return r == 0x6 ? 8 : 4;
    } else if ((b & 0xf8) == 0x90) {
        // SUB r
        uint8_t r = b & 0x7;
        DIS { printf("SUB %s\n", REG8N(r)); }

        uint8_t n = REG8(cpu, r);
        cpu->fz = cpu->a == n;
        cpu->fn = 1;
        cpu->fh = (((int) cpu->a & 0xf) - ((int) n & 0xf)) < 0;  // ?
        cpu->fc = cpu->a > n;
        cpu->a -= n;
        return r == 0x6 ? 8 : 4;
    } else if (b == 0xe6) {
        // AND n
        uint8_t n = GET8(cpu, cpu->pc++);
        DIS { printf("AND $%02x\n", n); }

        cpu->a &= n;
        cpu->f = 0;
        cpu->fh = 1;
        cpu->fz = cpu->a == 0;
        return 8;
    } else if ((b & 0xf8) == 0xa8) {
        // XOR r
        uint8_t r = b & 0x7;
        DIS { printf("XOR %s\n", REG8N(r)); }
        cpu->a = cpu->a ^ REG8(cpu, r);

        cpu->f = 0;
        cpu->fz = cpu->a == 0;
        return r == 0x6 ? 8 : 4;
    } else if ((b & 0xf8) == 0xb8) {
        // CP r
        uint8_t r = b & 0x7;
        DIS { printf("CP %s\n", REG8N(r)); }

        uint8_t n = REG8(cpu, r);
        cpu->fz = cpu->a == n;
        cpu->fn = 1;
        cpu->fh = (((int) cpu->a & 0xf) - ((int) n & 0xf)) < 0;  // ?
        cpu->fc = cpu->a > n;
        return r == 0x6 ? 8 : 4;
    } else if (b == 0xfe) {
        // CP n
        uint8_t n = GET8(cpu, cpu->pc++);
        DIS { printf("CP $%02x\n", n); }

        cpu->fz = cpu->a == n;
        cpu->fn = 1;
        cpu->fh = (((int) cpu->a & 0xf) - ((int) n & 0xf)) < 0;  // ?
        cpu->fc = cpu->a > n;
        return 8;
    } else if ((b & 0xc7) == 0x04) {
        // INC r
        uint8_t r = (b >> 3) & 0x7;
        DIS { printf("INC %s\n", REG8N(r)); }
        uint8_t v = REG8(cpu, r) + 1;
        SREG8(cpu, r, v);

        cpu->fz = v == 0;
        cpu->fn = 0;
        cpu->fh = v == 0x10;
        return r == 0x6 ? 12 : 4;
    } else if ((b & 0xc7) == 0x05) {
        // DEC r
        uint8_t r = (b >> 3) & 0x7;
        DIS { printf("DEC %s\n", REG8N(r)); }
        uint8_t v = REG8(cpu, r) - 1;
        SREG8(cpu, r, v);

        cpu->fz = v == 0;
        cpu->fn = 1;
        cpu->fh = (v & 0xf) == 0xf;
        return r == 0x6 ? 12 : 4;
    } else if ((b & 0xcf) == 0x03) {
        // INC ss
        uint8_t r = (b >> 4) & 0x3;
        DIS { printf("INC %s\n", REG16N(r)); }
        uint16_t v = REG16(cpu, r) + 1;
        SREG16(cpu, r, v);

        // no flags set (!)
        return 8;
    } else if ((b & 0xcf) == 0x0b) {
        // DEC ss
        uint8_t r = (b >> 4) & 0x3;
        DIS { printf("DEC %s\n", REG16N(r)); }
        uint16_t v = REG16(cpu, r) - 1;
        SREG16(cpu, r, v);

        // no flags set
        return 8;
    } else if (b == 0x17) {
        // RLA
        DIS { printf("RLA\n"); }
        uint8_t v = cpu->a,
                old_fc = cpu->fc;
        cpu->f = 0;
        cpu->fc = (v & 0x80) == 0x80;

        v = ((v & 0x7f) << 1) | old_fc;
        cpu->a = v;

        cpu->fz = cpu->a == 0;
        return 4;
    } else if (b == 0xcb) {
        b = GET8(cpu, cpu->pc++);
        if ((b & 0xf8) == 0x10) {
            // RL r
            uint8_t r = b & 0x7;
            DIS { printf("RL %s\n", REG8N(r)); }
            uint8_t v = REG8(cpu, r),
                    old_fc = cpu->fc;

            cpu->f = 0;
            cpu->fc = (v & 0x80) == 0x80;

            v = ((v & 0x7f) << 1) | old_fc;
            SREG8(cpu, r, v);

            cpu->fz = v == 0;
            return r == 0x6 ? 16 : 8;
        } else if ((b & 0xc0) == 0x40) {
            // BIT b,r
            uint8_t bit = (b >> 3) & 0x7,
                    r = b & 0x7;
            DIS { printf("BIT %d,%s\n", bit, REG8N(r)); }

            cpu->fz = ((REG8(cpu, r) >> bit) & 0x1) == 0;
            cpu->fn = 0;
            cpu->fh = 1;
            return r == 0x6 ? 12 : 8;
        } else if ((b & 0xc0) == 0x80) {
            // RES b,r
            uint8_t bit = (b >> 3) & 0x7,
                    r = b & 0x7;
            DIS { printf("RES %d,%s\n", bit, REG8N(r)); }
            SREG8(cpu, r, REG8(cpu, r) & ~(1 << bit));
            return r == 0x6 ? 16 : 8;
        } else {
            fprintf(stderr, "unknown cb opcode: %x\n", b);
            dump(cpu);
            return -1;
        }
    } else if (b == 0xc3) {
        // JP nn
        uint16_t v = GET8(cpu, cpu->pc++);
        v |= GET8(cpu, cpu->pc++) << 8;
        DIS { printf("JP $%04x\n", v); }
        cpu->pc = v;
        return 16;
    } else if (b == 0x18) {
        // JR e
        int16_t e = (int16_t) ((int8_t) GET8(cpu, cpu->pc++)) + 2;
        DIS { printf("JR %d\n", e); }
        cpu->pc += (-2) + e;
        return 12;
    } else if ((b & 0xe7) == 0x20) {
        // JR cc, e
        uint8_t cc = (b >> 3) & 0x3;
        int16_t e = (int16_t) ((int8_t) GET8(cpu, cpu->pc++)) + 2;
        DIS { printf("JR %s, %d\n", CCN(cc), e); }

        int do_jump = 0;
        switch (cc) {
            case 0x0: do_jump = !cpu->fz; break;
            case 0x1: do_jump = cpu->fz; break;
            case 0x2: do_jump = !cpu->fc; break;
            case 0x3: do_jump = cpu->fc; break;
        }

        if (do_jump) {
            cpu->pc += (-2) + e;
        }
        return 6;
    } else if (b == 0xcd) {
        // CALL nn
        uint16_t v = GET8(cpu, cpu->pc++);
        v |= GET8(cpu, cpu->pc++) << 8;
        DIS { printf("CALL $%04x\n", v); }
        PUSH16(cpu, cpu->pc);
        cpu->pc = v;

        // no flags set
        return 24;
    } else if (b == 0xc9) {
        // RET
        DIS { printf("RET\n"); }
        cpu->pc = POP16(cpu);
        return 16;
    } else if (b == 0x00) {
        // NOP
        DIS { printf("NOP\n"); }
        return 4;
    } else if (b == 0xf3) {
        // DI
        DIS { printf("DI\n"); }
        return 4;
    }

    fprintf(stderr, "unknown opcode: %x\n", b);
    dump(cpu);
    return -1;
}

// vim: set sw=4 et:
