#include <stdio.h>
#include <string.h>

#include "cpu.h"

void cpu_init(cpu_t *cpu, uint8_t const *rom, uint8_t *cart) {
    memset(cpu, 0, sizeof(*cpu));

    memcpy(cpu->rom, rom, 256);
    cpu->cart = cart;
    cpu->pc = 0;
    cpu->rom_lock = 1;
    cpu->rom_bank_selected = 1;
    cpu->lcdc = 0x83;
    cpu->lcdc_bgp = 0xd4;

    switch (rom[0x0147]) {
    case 0x00:
        cpu->mbc = 0;
        break;
    case 0x01:
    case 0x02:
    case 0x03:
        cpu->mbc = 1;
        break;
    case 0x05:
    case 0x06:
        cpu->mbc = 2;
        break;
    case 0x0f:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
        cpu->mbc = 3;
        break;
    case 0x19:
    case 0x1a:
    case 0x1b:
    case 0x1c:
    case 0x1d:
    case 0x1e:
        cpu->mbc = 5;
        break;
    default:
        fprintf(stderr, "unknown MBC %02x\n", rom[0x0147]);
        exit(1);
    }
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
    printf(" LCDC: %02x  BGP: %02x\n", cpu->lcdc, cpu->lcdc_bgp);
    printf("SCX/Y: %02x/%02x\n", cpu->lcdc_scx, cpu->lcdc_scy);
    printf("==========================\n");
    printf("\n");
}

uint8_t GET8(cpu_t const *cpu, uint16_t addr) {
    if (addr < 0x100 && cpu->rom_lock) {
        return cpu->rom[addr];
    } else if (addr < 0x4000) {
        return cpu->cart[addr];
    } else if (addr >= 0x4000 & addr < 0x8000) {
        switch (cpu->mbc) {
        case 0:
            return cpu->cart[addr];
        case 3:
            return cpu->cart[addr + (cpu->rom_bank_selected) * 0x4000];
        default:
            fprintf(stderr, "what does an mbc do %02x\n", cpu->mbc);
            exit(1);
        }
    } else if (addr == 0xff11) {
        // NR11
        return cpu->nr11;
    } else if (addr == 0xff12) {
        // NR12
        return cpu->nr12;
    } else if (addr == 0xff13) {
        // NR13
        return cpu->nr13;
    } else if (addr == 0xff14) {
        // NR14
        return cpu->nr14;
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
    } else if (addr == 0xff47) {
        // BGP
        return cpu->lcdc_bgp;
    }
    return cpu->ram[addr];
}

void SET8(cpu_t *cpu, uint16_t addr, uint8_t v) {
    if (addr == 0xff50) {
        if (v != 0) {
            cpu->rom_lock = 0;
            printf("DMG ROM overlay removed\n");
        }
        return;
    }

    if (cpu->mbc == 3) {
        if (addr >= 0x0000 & addr <= 0x1fff) {
            printf("write $%02x to $%04x\n", v, addr);
            exit(1);
        }

        if (addr >= 0x2000 && addr <= 0x3fff) {
            cpu->rom_bank_selected = v & 0x7f;
            if (!cpu->rom_bank_selected) {
                cpu->rom_bank_selected = 1;
            }
            return;
        }

        if (addr >= 0x4000 && addr <= 0x5fff) {
            printf("write $%02x to $%04x\n", v, addr);
            exit(1);
        }

        if (addr >= 0x6000 && addr <= 0x7fff) {
            printf("write $%02x to $%04x\n", v, addr);
            exit(1);
        }
    }

    if (addr == 0xff11) {
        // NR11
        cpu->nr11 = v;
    } else if (addr == 0xff12) {
        // NR12
        cpu->nr12 = v;
    } else if (addr == 0xff13) {
        // NR13
        cpu->nr13 = v;
    } else if (addr == 0xff14) {
        // NR14
        cpu->nr14 = v;
    } else if (addr == 0xff40) {
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
    } else if (addr == 0xff47) {
        // BGP
        cpu->lcdc_bgp = v;
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

struct daa_table_entry {
    int fc_before;
    int bit47_low;
    int bit47_high;
    int fh_before;
    int bit03_low;
    int bit03_high;
    int add_a;
    int set_fc;
};

struct daa_table_entry daa_add[9] = {
    { 0, 0x0, 0x9, 0, 0x0, 0x9, 0x00, 0 },
    { 0, 0x0, 0x8, 0, 0xa, 0xf, 0x06, 0 },
    { 0, 0x0, 0x9, 1, 0x0, 0x3, 0x06, 0 },
    { 0, 0xa, 0xf, 0, 0x0, 0x9, 0x60, 1 },
    { 0, 0x9, 0xf, 0, 0xa, 0xf, 0x66, 1 },
    { 0, 0xa, 0xf, 1, 0x0, 0x3, 0x66, 1 },
    { 1, 0x0, 0x2, 0, 0x0, 0x9, 0x60, 1 },
    { 1, 0x0, 0x2, 0, 0xa, 0xf, 0x66, 1 },
    { 1, 0x0, 0x3, 1, 0x0, 0x3, 0x66, 1 },
};

struct daa_table_entry daa_sub[4] = {
    { 0, 0x0, 0x9, 0, 0x0, 0x9, 0x00, 0 },
    { 0, 0x0, 0x8, 1, 0x6, 0xf, 0xfa, 0 },
    { 1, 0x7, 0xf, 0, 0x0, 0x9, 0xa0, 1 },
    { 1, 0x6, 0xf, 1, 0x6, 0xf, 0x9a, 1},
};

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
    } else if (b == 0x0a) {
        // LD A,(BC)
        DIS { printf("LD A,(BC)\n"); }
        cpu->a = GET8(cpu, cpu->bc);

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
    } else if (b == 0xfa) {
        // LD A,(nn)
        uint16_t v = GET8(cpu, cpu->pc++);
        v |= GET8(cpu, cpu->pc++) << 8;
        DIS { printf("LD A,($%04x)\n", v); }
        cpu->a = GET8(cpu, v);

        // no flags set
        return 16;
    } else if (b == 0xea) {
        // LD (nn),A
        uint16_t v = GET8(cpu, cpu->pc++);
        v |= GET8(cpu, cpu->pc++) << 8;
        DIS { printf("LD ($%04x),A\n", v); }
        SET8(cpu, v, cpu->a);

        // no flags set
        return 16;
    } else if (b == 0x2a) {
        // LD A,(HL+)
        DIS { printf("LD A,(HL+)\n"); }
        cpu->a = GET8(cpu, cpu->hl++);

        // no flags set
        return 8;
    } else if (b == 0x3a) {
        // LD A,(HL-)
        DIS { printf("LD A,(HL-)\n"); }
        cpu->a = GET8(cpu, cpu->hl--);

        // no flags set
        return 8;
    } else if (b == 0x02) {
        // LD (BC),A
        DIS { printf("LD (BC),A\n"); }
        SET8(cpu, cpu->bc, cpu->a);

        // no flags set
        return 8;
    } else if (b == 0x12) {
        // LD (DE),A
        DIS { printf("LD (DE),A\n"); }
        SET8(cpu, cpu->de, cpu->a);

        // no flags set
        return 8;
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
    } else if (b == 0x08) {
        // LD (nn),SP
        uint16_t v = GET8(cpu, cpu->pc++);
        v |= GET8(cpu, cpu->pc++) << 8;
        DIS { printf("LD ($%04x),SP\n", v); }

        SET8(cpu, v, cpu->sp & 0xff);
        SET8(cpu, v + 1, cpu->sp >> 8);

        // no flags set
        return 20;
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
    } else if ((b & 0xf8) == 0xa0) {
        // AND r
        uint8_t r = b & 0x7;
        DIS { printf("AND %s\n", REG8N(r)); }

        cpu->a &= REG8(cpu, r);
        cpu->f = 0;
        cpu->fh = 1;
        cpu->fz = cpu->a == 0;
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
    } else if ((b & 0xf8) == 0xb0) {
        // OR r
        uint8_t r = b & 0x7;
        DIS { printf("OR %s\n", REG8N(r)); }

        cpu->a |= REG8(cpu, r);
        cpu->f = 0;
        cpu->fz = cpu->a == 0;
        return r == 0x6 ? 8 : 4;
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
    } else if ((b & 0xcf) == 0x09) {
        // ADD HL,ss
        uint8_t r = (b >> 4) & 0x3;
        DIS { printf("ADD HL,%s\n", REG16N(r)); }
        uint16_t n = REG16(cpu, r);
        cpu->fh = (((n & 0xfff) + (cpu->hl & 0xfff)) & 0x1000) == 0x1000;
        cpu->fn = 0;
        cpu->fc = ((uint32_t) n) + ((uint32_t) cpu->hl) > 0xffff;
        cpu->hl += n;
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
    } else if (b == 0x07) {
        // RLCA
        DIS { printf("RLCA\n"); }
        uint8_t v = cpu->a;
        cpu->f = 0;
        cpu->fc = (v & 0x80) == 0x80;

        v = ((v & 0x7f) << 1) | (v >> 7);
        cpu->a = v;

        cpu->fz = cpu->a == 0;
        return 4;
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
    } else if (b == 0x0f) {
        // RRCA
        DIS { printf("RRCA\n"); }
        uint8_t v = cpu->a;
        cpu->f = 0;
        cpu->fc = (v & 0x1) == 0x1;

        v = ((v & 0xfe) >> 1) | ((v & 0x1) << 7);
        cpu->a = v;

        cpu->fz = cpu->a == 0;
        return 4;
    } else if (b == 0x1f) {
        // RRA
        DIS { printf("RRA\n"); }
        uint8_t v = cpu->a,
                old_fc = cpu->fc;
        cpu->f = 0;
        cpu->fc = (v & 0x1) == 0x1;

        v = ((v & 0xfe) >> 1) | (old_fc << 7);
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
        } else if ((b & 0xf8) == 0x18) {
            // RR r
            uint8_t r = b & 0x7;
            DIS { printf("RR %s\n", REG8N(r)); }
            uint8_t v = REG8(cpu, r),
                    old_fc = cpu->fc;

            cpu->f = 0;
            cpu->fc = (v & 0x1) == 0x1;

            v = ((v & 0xfe) >> 1) | (old_fc << 7);
            SREG8(cpu, r, v);

            cpu->fz = v == 0;
            return r == 0x6 ? 16 : 8;
        } else if ((b & 0xf8) == 0x30) {
            // SWAP r
            uint8_t r = b & 0x7;
            DIS { printf("SWAP %s\n", REG8N(r)); }

            uint8_t v = REG8(cpu, r);
            v = (v >> 4) | (v << 4);
            SREG8(cpu, r, v);

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
        } else if ((b & 0xc0) == 0xc0) {
            // SET b,r
            uint8_t bit = (b >> 3) & 0x7,
                    r = b & 0x7;
            DIS { printf("RES %d,%s\n", bit, REG8N(r)); }
            SREG8(cpu, r, REG8(cpu, r) | (1 << bit));
            return r == 0x6 ? 16 : 8;
        } else if ((b & 0xc0) == 0x80) {
            // RES b,r
            uint8_t bit = (b >> 3) & 0x7,
                    r = b & 0x7;
            DIS { printf("RES %d,%s\n", bit, REG8N(r)); }
            SREG8(cpu, r, REG8(cpu, r) & ~(1 << bit));
            return r == 0x6 ? 16 : 8;
        } else {
            fprintf(stderr, "unknown CB opcode: %x\n", b);
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
    } else if ((b & 0xe7) == 0xc2) {
        // JP cc,nn
        uint8_t cc = (b >> 3) & 0x3;
        uint16_t v = GET8(cpu, cpu->pc++);
        v |= GET8(cpu, cpu->pc++) << 8;
        DIS { printf("JP %s,$%04x\n", CCN(cc), v); }

        int do_jump = 0;
        switch (cc) {
            case 0x0: do_jump = !cpu->fz; break;
            case 0x1: do_jump = cpu->fz; break;
            case 0x2: do_jump = !cpu->fc; break;
            case 0x3: do_jump = cpu->fc; break;
        }

        if (do_jump) {
            cpu->pc = v;
            return 16;
        }
        return 12;
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
            return 12;
        }
        return 8;
    } else if (b == 0xe9) {
        // JP (HL)
        DIS { printf("JP (HL)\n"); }

        cpu->pc = cpu->hl;
        return 4;
    } else if (b == 0xcd) {
        // CALL nn
        uint16_t v = GET8(cpu, cpu->pc++);
        v |= GET8(cpu, cpu->pc++) << 8;
        DIS { printf("CALL $%04x\n", v); }
        PUSH16(cpu, cpu->pc);
        cpu->pc = v;

        // no flags set
        return 24;
    } else if ((b & 0xe7) == 0xc4) {
        // CALL cc,nn
        uint8_t cc = (b >> 3) & 0x3;
        uint16_t v = GET8(cpu, cpu->pc++);
        v |= GET8(cpu, cpu->pc++) << 8;
        DIS { printf("CALL %s,$%04x\n", CCN(cc), v); }

        int do_jump = 0;
        switch (cc) {
            case 0x0: do_jump = !cpu->fz; break;
            case 0x1: do_jump = cpu->fz; break;
            case 0x2: do_jump = !cpu->fc; break;
            case 0x3: do_jump = cpu->fc; break;
        }

        if (do_jump) {
            PUSH16(cpu, cpu->pc);
            cpu->pc = v;
            return 20;
        }

        // no flags set
        return 8;
    } else if (b == 0xc9) {
        // RET
        DIS { printf("RET\n"); }
        cpu->pc = POP16(cpu);

        // no flags set
        return 16;
    } else if ((b & 0xe7) == 0xc0) {
        // RET cc
        uint8_t cc = (b >> 3) & 0x3;
        DIS { printf("RET %s\n", CCN(cc)); }

        int do_jump = 0;
        switch (cc) {
            case 0x0: do_jump = !cpu->fz; break;
            case 0x1: do_jump = cpu->fz; break;
            case 0x2: do_jump = !cpu->fc; break;
            case 0x3: do_jump = cpu->fc; break;
        }

        if (do_jump) {
            cpu->pc = POP16(cpu);
            return 20;
        }

        // no flags set
        return 8;
    } else if ((b & 0xc7) == 0xc7) {
        // RST t
        uint8_t t = (b >> 3) & 0x7;
        DIS { printf("RST %d\n", t); }
        PUSH16(cpu, cpu->pc);
        cpu->pc = t * 8;
        // no flags set
        return 16;
    } else if (b == 0x27) {
        // DAA
        DIS { printf("DAA\n"); }

        cpu->fh = 0;

        struct daa_table_entry *table;
        int tablelen;
        if (!cpu->fn) {
            table = daa_add;
            tablelen = 9;
        } else {
            table = daa_sub;
            tablelen = 4;
        }

        uint8_t bit47 = cpu->a >> 4,
                bit03 = cpu->a & 0xf;
        int i;
        for (i = 0; i < tablelen; ++i) {
            if (
                cpu->fc == table[i].fc_before &&
                bit47 >= table[i].bit47_low &&
                bit47 <= table[i].bit47_high &&
                cpu->fh == table[i].fh_before &&
                bit03 >= table[i].bit03_low &&
                bit03 <= table[i].bit03_high
            ) {
                cpu->a += table[i].add_a;
                cpu->fc = table[i].set_fc;
                break;
            }
        }

        if (i == tablelen) {
            fprintf(stderr, "DAA hit wall on %02x\n", cpu->a);
            exit(1);
        }

        return 4;
    } else if (b == 0x3f) {
        // CCF
        DIS { printf("CCF\n"); }
        cpu->fn = cpu->fh = 0;
        cpu->fc = !cpu->fc;
        return 4;
    } else if (b == 0x37) {
        // SCF
        DIS { printf("SCF\n"); }
        cpu->fn = cpu->fh = 0;
        cpu->fc = 1;
        return 4;
    } else if (b == 0x00) {
        // NOP
        DIS { printf("NOP\n"); }
        // no flags set
        return 4;
    } else if (b == 0xf3) {
        // DI
        DIS { printf("DI\n"); }
        // no flags set
        return 4;
    } else if (b == 0xfb) {
        // EI
        DIS { printf("EI\n"); }
        // no flags set
        return 4;
    }

    fprintf(stderr, "unknown opcode: %x\n", b);
    dump(cpu);
    return -1;
}

// vim: set sw=4 et:
