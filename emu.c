#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int read_file(char const *filename, uint8_t **out, long *len) {
    FILE *f = fopen(filename, "r");
    fseek(f, 0, SEEK_END);
    *len = ftell(f);
    fseek(f, 0, SEEK_SET);

    *out = malloc(*len);
    fread(*out, 1, *len, f);
    fclose(f);

    return 0;
}

typedef struct {
    uint8_t a;
    union {
        struct {
            unsigned int f0 : 4;
            unsigned int fc : 1;
            unsigned int fh : 1;
            unsigned int fn : 1;
            unsigned int fz : 1;
        };
        uint8_t f;
    };
    union {
        struct {
            uint8_t c, b;
        };
        uint16_t bc;
    };
    union {
        struct {
            uint8_t e, d;
        };
        uint16_t de;
    };
    union {
       struct {
          uint8_t l, h;
       };
       uint16_t hl;
    };
    uint16_t sp, pc;

    uint8_t rom[0x100], ram[0x10000];
} cpu_t;

void dump(cpu_t const *cpu) {
    printf("\n");
    printf("======== CPU DUMP ========\n");
    printf(" A: %02x      F: %02x\n", cpu->a, cpu->f);
    printf(" B: %02x      C: %02x\n", cpu->b, cpu->c);
    printf(" D: %02x      E: %02x\n", cpu->d, cpu->e);
    printf(" H: %02x      L: %02x\n", cpu->h, cpu->l);
    printf("SP: %04x   PC: %04x\n", cpu->sp, cpu->pc);
    printf("==========================\n");
    printf("\n");
}

uint8_t GET8(cpu_t const *cpu, uint16_t addr) {
    if (addr < 0x100 && cpu->ram[0xff50] == 0x00) {
        return cpu->rom[addr];
    }
    return cpu->ram[addr];
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

void SET8(cpu_t *cpu, uint16_t addr, uint8_t v) {
    cpu->ram[addr] = v;
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

int main(int argc, char **argv) {
    uint8_t *rom, *cart;
    long romlen, cartlen;
    read_file("DMG_ROM.bin", &rom, &romlen);
    read_file("red.gb", &cart, &cartlen);

    if (romlen != 256) {
        fprintf(stderr, "ROM not 256 bytes; aborting\n");
        return 1;
    }

    cpu_t cpu;
    memcpy(cpu.rom, rom, 256);
    cpu.pc = 0;
    SET8(&cpu, 0xff50, 0);

    dump(&cpu);

    int show_dis = 0;
#define DIS if (show_dis)

    while (1) {
        if (cpu.pc == 0x40 && !show_dis) {
            printf("...\n");
            show_dis = 1;
        }

        DIS { printf("%04x: ", cpu.pc); }

        uint8_t b = GET8(&cpu, cpu.pc++);

        if ((b & 0xc7) == 0x06) {
            // LD r,n
            uint8_t r = (b >> 3) & 0x7;
            uint8_t v = GET8(&cpu, cpu.pc++);
            DIS { printf("LD %s,$%x\n", REG8N(r), v); }
            SREG8(&cpu, r, v);

            // no flags set
        } else if (b == 0xe2) {
            // LD ($FF00+C),A
            DIS { printf("LD ($FF00+C),A\n"); }
            SET8(&cpu, 0xff00 + cpu.c, cpu.a);

            // no flags set
        } else if (b == 0xfe) {
            // CP n
            uint8_t n = GET8(&cpu, cpu.pc++);
            DIS { printf("CP $%02x\n", n); }

            cpu.fz = cpu.a == n;
            cpu.fn = 1;
            cpu.fh = (((int) cpu.a & 0xf) - ((int) n & 0xf)) < 0;  // ?
            cpu.fc = cpu.a > n;
        } else if ((b & 0xc7) == 0x04) {
            // INC r
            uint8_t r = (b >> 3) & 0x7;
            DIS { printf("INC %s\n", REG8N(r)); }
            uint8_t v = REG8(&cpu, r) + 1;
            SREG8(&cpu, r, v);

            cpu.fz = v == 0;
            cpu.fn = 0;
            cpu.fh = v == 0x10;
        } else if ((b & 0xc7) == 0x05) {
            // DEC r
            uint8_t r = (b >> 3) & 0x7;
            DIS { printf("DEC %s\n", REG8N(r)); }
            uint8_t v = REG8(&cpu, r) - 1;
            SREG8(&cpu, r, v);

            cpu.fz = v == 0;
            cpu.fn = 1;
            cpu.fh = (v & 0xf) == 0xf;
        } else if ((b & 0xcf) == 0x03) {
            // INC ss
            uint8_t r = (b >> 4) & 0x3;
            DIS { printf("INC %s\n", REG16N(r)); }
            uint16_t v = REG16(&cpu, r) + 1;
            SREG16(&cpu, r, v);

            // no flags set (!)
        } else if ((b & 0xf8) == 0x70) {
            // LD (HL),r
            uint8_t r = b & 0x7;
            DIS { printf("LD (HL),%s\n", REG8N(r)); }
            SET8(&cpu, cpu.hl, REG8(&cpu, r));

            // no flags set
        } else if (b == 0xe0) {
            // LD ($FF00+n),A
            uint8_t n = GET8(&cpu, cpu.pc++);
            DIS { printf("LD ($FF00+$%x),A\n", n); }
            SET8(&cpu, 0xff00 + n, cpu.a);

            // no flags set
        } else if (b == 0x1a) {
            // LD A,(DE)
            DIS { printf("LD A,(DE)\n"); }
            cpu.a = GET8(&cpu, cpu.de);

            // no flags set
        } else if ((b & 0xc0) == 0x40) {
            // LD r,r'
            uint8_t dst = (b >> 3) & 0x7,
                    src = b & 0x7;
            DIS { printf("LD %s,%s\n", REG8N(dst), REG8N(src)); }
            SREG8(&cpu, dst, REG8(&cpu, src));

            // no flags set
        } else if ((b & 0xcf) == 0xc5) {
            // PUSH qq
            uint8_t qq = (b >> 4) & 0x3;
            DIS { printf("PUSH %s\n", REG16N(qq)); }
            PUSH16(&cpu, REG16(&cpu, qq));

            // no flags set
        } else if ((b & 0xcf) == 0xc1) {
            // POP qq
            uint8_t qq = (b >> 4) & 0x3;
            DIS { printf("POP %s\n", REG16N(qq)); }
            SREG16(&cpu, qq, POP16(&cpu));

            // no flags set
        } else if (b == 0xea) {
            // LD (nn),A
            uint16_t v = GET8(&cpu, cpu.pc++);
            v |= GET8(&cpu, cpu.pc++) << 8;
            DIS { printf("LD ($%04x),A\n", v); }
            SET8(&cpu, v, cpu.a);

            // no flags set
        } else if ((b & 0xcf) == 0x01) {
            // LD dd,nn
            uint8_t r = (b >> 4) & 0x3;
            uint16_t v = GET8(&cpu, cpu.pc++);
            v |= GET8(&cpu, cpu.pc++) << 8;
            DIS { printf("LD %s,$%04x\n", REG16N(r), v); }
            SREG16(&cpu, r, v);

            // no flags set
        } else if ((b & 0xf8) == 0xa8) {
            // XOR r
            DIS { printf("XOR %s\n", REG8N(b & 0x7)); }
            cpu.a = cpu.a ^ REG8(&cpu, b & 0x7);

            cpu.f = 0;
            cpu.fz = cpu.a == 0;
        } else if (b == 0x32) {
            // LD (HL-),A
            DIS { printf("LD (HL-),A\n"); }
            SET8(&cpu, cpu.hl--, cpu.a);

            // no flags set
        } else if (b == 0x22) {
            // LD (HL+),A
            DIS { printf("LD (HL+),A\n"); }
            SET8(&cpu, cpu.hl++, cpu.a);

            // no flags set
        } else if (b == 0x17) {
            // RLA
            DIS { printf("RLA\n"); }
            uint8_t v = cpu.a,
                    old_fc = cpu.fc;
            cpu.f = 0;
            cpu.fc = (v & 0x80) == 0x80;

            v = ((v & 0x7f) << 1) | old_fc;
            cpu.a = v;

            cpu.fz = cpu.a == 0;
        } else if (b == 0xcb) {
            b = GET8(&cpu, cpu.pc++);
            if ((b & 0xc0) == 0x40) {
                // BIT b,r
                uint8_t bit = (b >> 3) & 0x7,
                        r = b & 0x7;
                DIS { printf("BIT %d,%s\n", bit, REG8N(r)); }

                cpu.fz = ((REG8(&cpu, r) >> bit) & 0x1) == 0;
                cpu.fn = 0;
                cpu.fh = 1;
            } else if ((b & 0xf8) == 0x10) {
                // RL r
                uint8_t r = b & 0x3;
                DIS { printf("RL %s\n", REG8N(r)); }
                uint8_t v = REG8(&cpu, r),
                        old_fc = cpu.fc;

                cpu.f = 0;
                cpu.fc = (v & 0x80) == 0x80;

                v = ((v & 0x7f) << 1) | old_fc;
                SREG8(&cpu, r, v);

                cpu.fz = v == 0;
            } else {
                fprintf(stderr, "unknown cb opcode: %x\n", b);
                dump(&cpu);
                return 1;
            }
        } else if ((b & 0xe7) == 0x20) {
            // JR cc, e
            uint8_t cc = (b >> 3) & 0x3;
            int16_t e = (int16_t) ((int8_t) GET8(&cpu, cpu.pc++)) + 2;
            DIS { printf("JR %s, %d\n", CCN(cc), e); }

            int do_jump = 0;
            switch (cc) {
                case 0x0: do_jump = !cpu.fz; break;
                case 0x1: do_jump = cpu.fz; break;
                case 0x2: do_jump = !cpu.fc; break;
                case 0x3: do_jump = cpu.fc; break;
            }

            if (do_jump) {
                cpu.pc += (-2) + e;
            }
        } else if (b == 0xcd) {
            // CALL nn
            uint16_t v = GET8(&cpu, cpu.pc++);
            v |= GET8(&cpu, cpu.pc++) << 8;
            DIS { printf("CALL $%04x\n", v); }
            PUSH16(&cpu, cpu.pc);
            cpu.pc = v;

            // no flags set
        } else if (b == 0xc9) {
            // RET
            DIS { printf("RET\n"); }
            cpu.pc = POP16(&cpu);
        } else {
            fprintf(stderr, "unknown opcode: %x\n", b);
            dump(&cpu);
            return 1;
        }
    }

    return 0;
}

// vim: set sw=4 et:
