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
    // f: Z, N, H, C, 0, 0, 0, 0
    uint8_t a, f, b, c, d, e, h, l;
    uint16_t sp, pc;

    uint8_t ff50;
} cpu_t;

void dump(cpu_t const *cpu) {
    printf("======== CPU DUMP ========\n");
    printf(" A: %02x      F: %02x\n", cpu->a, cpu->f);
    printf(" B: %02x      C: %02x\n", cpu->b, cpu->c);
    printf(" D: %02x      E: %02x\n", cpu->d, cpu->e);
    printf(" H: %02x      L: %02x\n", cpu->h, cpu->l);
    printf("SP: %04x   PC: %04x\n", cpu->sp, cpu->pc);
    printf("==========================\n");
    printf("\n");
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

    while (1) {
        uint8_t b = rom[cpu.pc++];

        if ((b & 0xcf) == 0x01) {
            switch ((b >> 4) & 0x3) {
                case 0x00:
                    cpu.b = rom[cpu.pc++];
                    cpu.c = rom[cpu.pc++];
                    break;
                case 0x01:
                    cpu.d = rom[cpu.pc++];
                    cpu.e = rom[cpu.pc++];
                    break;
                case 0x02:
                    cpu.h = rom[cpu.pc++];
                    cpu.l = rom[cpu.pc++];
                    break;
                case 0x03:
                    cpu.sp = rom[cpu.pc++] << 8;
                    cpu.sp |= rom[cpu.pc++];
                    break;
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
