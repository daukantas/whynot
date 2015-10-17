#ifndef CPU_H
#define CPU_H

#include <stdlib.h>

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

    uint8_t rom[0x100], ram[0x10000], *cart;
} cpu_t;

void cpu_init(cpu_t *cpu, uint8_t const *rom, uint8_t *cart);

void dump(cpu_t const *cpu);

uint8_t GET8(cpu_t const *cpu, uint16_t addr);
void SET8(cpu_t *cpu, uint16_t addr, uint8_t v);

uint8_t REG8(cpu_t const *cpu, int s);
void SREG8(cpu_t *cpu, int s, uint8_t v);
char const *REG8N(int s);

uint16_t REG16(cpu_t *cpu, int s);
void SREG16(cpu_t *cpu, int s, uint16_t v);
char const *REG16N(int s);

char const *CCN(int s);

void PUSH16(cpu_t *cpu, uint16_t v);
uint16_t POP16(cpu_t *cpu);

#endif

// vim: set sw=4 et:
