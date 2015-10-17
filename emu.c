#include <stdio.h>
#include <stdlib.h>

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

    uint8_t ram[0x10000];
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

uint8_t REG8(cpu_t const *cpu, int s) {
    switch (s) {
        case 0x7: return cpu->a;
        case 0x0: return cpu->b;
        case 0x1: return cpu->c;
        case 0x2: return cpu->d;
        case 0x3: return cpu->e;
        case 0x4: return cpu->h;
        case 0x5: return cpu->l;
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
        default:
            fprintf(stderr, "REG8N got %x\n", s);
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
    cpu.pc = 0;
    cpu.ram[0xff50] = 0;

    dump(&cpu);

    int show_dis = 0;
#define DIS if (show_dis)

    while (1) {
        if (cpu.pc >= 0x14 && !show_dis) {
            printf("...\n");
            show_dis = 1;
        }

        uint8_t b = rom[cpu.pc++];

        if ((b & 0xc7) == 0x06) {
            // LD r,n
            uint8_t r = (b >> 3) & 0x7;
            uint8_t v = rom[cpu.pc++];
            DIS { printf("LD %s,$%x\n", REG8N(r), v); }
            SREG8(&cpu, r, v);

            // no flags set
        } else if (b == 0xe2) {
            // LD ($FF00+C),A
            DIS { printf("LD ($FF00+C),A\n"); }
            SET8(&cpu, 0xff00 + cpu.c, cpu.a);

            // no flags set
        } else if ((b & 0xc7) == 0x04) {
            // INC r
            uint8_t r = (b >> 3) & 0x7;
            DIS { printf("INC %s\n", REG8N(r)); }
            uint8_t v = REG8(&cpu, r) + 1;
            SREG8(&cpu, r, v);

            cpu.fz = v == 0;
            cpu.fn = 0;
            cpu.fh = v == 0x10;
        } else if ((b & 0xf8) == 0x70) {
            // LD (HL),r
            uint8_t r = b & 0x7;
            DIS { printf("LD (HL),%s\n", REG8N(r)); }
            SET8(&cpu, cpu.hl, REG8(&cpu, r));
        } else if (b == 0xe0) {
            // LD ($FF00+n),A
            uint8_t n = rom[cpu.pc++];
            DIS { printf("LD ($FF00+$%x),A\n", n); }
            SET8(&cpu, 0xff00 + n, cpu.a);
        } else if ((b & 0xcf) == 0x01) {
            // LD dd,nn
            uint8_t r = (b >> 4) & 0x3;
            uint16_t v = rom[cpu.pc++];
            v |= rom[cpu.pc++] << 8;
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
        } else if (b == 0xcb) {
            b = rom[cpu.pc++];
            if ((b & 0xc0) == 0x40) {
                // BIT b,r
                uint8_t bit = (b >> 3) & 0x7,
                        r = b & 0x7;
                DIS { printf("BIT %d,%s\n", bit, REG8N(r)); }

                cpu.fz = ((REG8(&cpu, r) >> bit) & 0x1) == 0;
                cpu.fn = 0;
                cpu.fh = 1;
            } else {
                fprintf(stderr, "unknown cb opcode: %x\n", b);
                dump(&cpu);
                return 1;
            }
        } else if ((b & 0xe7) == 0x20) {
            // JR cc, e
            uint8_t cc = (b >> 3) & 0x3;
            int16_t e = (int16_t) ((int8_t) rom[cpu.pc++]) + 2;
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
        } else {
            fprintf(stderr, "unknown opcode: %x\n", b);
            dump(&cpu);
            return 1;
        }
    }

    return 0;
}

// vim: set sw=4 et:
