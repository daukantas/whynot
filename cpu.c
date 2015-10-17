#include <stdio.h>

#include "cpu.h"

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
    if (addr < 0x8000) {
        return cpu->cart[addr];
    }
    return cpu->ram[addr];
}

void SET8(cpu_t *cpu, uint16_t addr, uint8_t v) {
    if (addr == 0xff50 && v != 0) {
        printf("DMG ROM overlay disabled\n");
    }
    cpu->ram[addr] = v;
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

