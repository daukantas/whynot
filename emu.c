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

    uint8_t ff50;
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

void SET8(cpu_t *cpu, uint16_t addr, uint8_t v) {
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
    cpu.ff50 = 0;

    dump(&cpu);

    // A 111 7
    // B 000 0
    // C 001 1
    // D 010 2
    // E 011 3
    // H 100 4
    // L 101 5

    while (1) {
        uint8_t b = rom[cpu.pc++];

        if ((b & 0xcf) == 0x01) {
            // LD dd, nn
            uint16_t v = rom[cpu.pc++];
            v |= rom[cpu.pc++] << 8;
            printf("LD %s,$%04x\n", REG16N((b >> 4) & 0x3), v);
            SREG16(&cpu, (b >> 4) & 0x3, v);
        } else if ((b & 0xf8) == 0xa8) {
            // XOR r
            printf("XOR %s\n", REG8N(b & 0x7));
            cpu.a = cpu.a ^ REG8(&cpu, b & 0x7);
            cpu.fz = cpu.a == 0;
        } else if (b == 0x32) {
            // LD (HLD), A
            printf("LD (HLD), A\n");
            SET8(&cpu, cpu.hl--, cpu.a);
        } else {
            fprintf(stderr, "unknown opcode: %x\n", b);
            dump(&cpu);
            return 1;
        }
    }

    return 0;
}

// vim: set sw=4 et:
