#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include "cpu.h"

int run(cpu_t *cpu);
int step(cpu_t *cpu);

#define SCRH 160
#define SCRW 144
#define SCRSCALE 3

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
    cpu_init(&cpu, rom, cart);

    SDL_Window *window = SDL_CreateWindow(
        "whynot",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCRW * SCRSCALE, SCRH * SCRSCALE,
        SDL_WINDOW_OPENGL);

    if (!window) {
        fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glcontext = SDL_GL_CreateContext(window);

    int retval = run(&cpu);

    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return retval;
}

static int show_dis = 0;
#define DIS if (show_dis)

int run(cpu_t *cpu) {
    dump(cpu);

    printf("starting execution\n");

    int running = 1;
    int retval = 0;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYDOWN:
                    if (event.key.repeat) {
                        break;
                    }

                    // keydown(event.key.keysym.sym);
                    break;

                case SDL_KEYUP:
                    // keyup(event.key.keysym.sym);
                    break;

                case SDL_QUIT:
                    running = 0;
                    break;
            }
        }

        if (step(cpu) == 1) {
            running = 0;
            retval = 1;
        }
    }
    return retval;
}

int step(cpu_t *cpu) {
    if (cpu->pc == 0x0100 && !show_dis) {
        printf("...\n");
        show_dis = 1;
    }

    DIS { printf("%04x: ", cpu->pc); }

    uint8_t b = GET8(cpu, cpu->pc++);

    if ((b & 0xc0) == 0x40) {
        // LD r,r'
        uint8_t dst = (b >> 3) & 0x7,
                src = b & 0x7;
        DIS { printf("LD %s,%s\n", REG8N(dst), REG8N(src)); }
        SREG8(cpu, dst, REG8(cpu, src));

        // no flags set
    } else if ((b & 0xc7) == 0x06) {
        // LD r,n
        uint8_t r = (b >> 3) & 0x7;
        uint8_t v = GET8(cpu, cpu->pc++);
        DIS { printf("LD %s,$%x\n", REG8N(r), v); }
        SREG8(cpu, r, v);

        // no flags set
    } else if (b == 0x1a) {
        // LD A,(DE)
        DIS { printf("LD A,(DE)\n"); }
        cpu->a = GET8(cpu, cpu->de);

        // no flags set
    } else if (b == 0xe2) {
        // LD ($FF00+C),A
        DIS { printf("LD ($FF00+C),A\n"); }
        SET8(cpu, 0xff00 + cpu->c, cpu->a);

        // no flags set
    } else if (b == 0xf0) {
        // LD A,($FF00+n)
        uint8_t n = GET8(cpu, cpu->pc++);
        DIS { printf("LD A,($FF00+$%x)\n", n); }
        cpu->a = GET8(cpu, 0xff00 + n);

        // no flags set
    } else if (b == 0xe0) {
        // LD ($FF00+n),A
        uint8_t n = GET8(cpu, cpu->pc++);
        DIS { printf("LD ($FF00+$%x),A\n", n); }
        SET8(cpu, 0xff00 + n, cpu->a);

        // no flags set
    } else if (b == 0xea) {
        // LD (nn),A
        uint16_t v = GET8(cpu, cpu->pc++);
        v |= GET8(cpu, cpu->pc++) << 8;
        DIS { printf("LD ($%04x),A\n", v); }
        SET8(cpu, v, cpu->a);

        // no flags set
    } else if (b == 0x22) {
        // LD (HL+),A
        DIS { printf("LD (HL+),A\n"); }
        SET8(cpu, cpu->hl++, cpu->a);

        // no flags set
    } else if (b == 0x32) {
        // LD (HL-),A
        DIS { printf("LD (HL-),A\n"); }
        SET8(cpu, cpu->hl--, cpu->a);

        // no flags set
    } else if ((b & 0xcf) == 0x01) {
        // LD dd,nn
        uint8_t r = (b >> 4) & 0x3;
        uint16_t v = GET8(cpu, cpu->pc++);
        v |= GET8(cpu, cpu->pc++) << 8;
        DIS { printf("LD %s,$%04x\n", REG16N(r), v); }
        SREG16(cpu, r, v);

        // no flags set
    } else if ((b & 0xcf) == 0xc5) {
        // PUSH qq
        uint8_t qq = (b >> 4) & 0x3;
        DIS { printf("PUSH %s\n", REG16N(qq)); }
        PUSH16(cpu, REG16(cpu, qq));

        // no flags set
    } else if ((b & 0xcf) == 0xc1) {
        // POP qq
        uint8_t qq = (b >> 4) & 0x3;
        DIS { printf("POP %s\n", REG16N(qq)); }
        SREG16(cpu, qq, POP16(cpu));

        // no flags set
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
    } else if ((b & 0xf8) == 0xa8) {
        // XOR r
        DIS { printf("XOR %s\n", REG8N(b & 0x7)); }
        cpu->a = cpu->a ^ REG8(cpu, b & 0x7);

        cpu->f = 0;
        cpu->fz = cpu->a == 0;
    } else if ((b & 0xf8) == 0xb8) {
        // CP r
        uint8_t r = b & 0x7;
        DIS { printf("CP %s\n", REG8N(r)); }

        uint8_t n = REG8(cpu, r);
        cpu->fz = cpu->a == n;
        cpu->fn = 1;
        cpu->fh = (((int) cpu->a & 0xf) - ((int) n & 0xf)) < 0;  // ?
        cpu->fc = cpu->a > n;
    } else if (b == 0xfe) {
        // CP n
        uint8_t n = GET8(cpu, cpu->pc++);
        DIS { printf("CP $%02x\n", n); }

        cpu->fz = cpu->a == n;
        cpu->fn = 1;
        cpu->fh = (((int) cpu->a & 0xf) - ((int) n & 0xf)) < 0;  // ?
        cpu->fc = cpu->a > n;
    } else if ((b & 0xc7) == 0x04) {
        // INC r
        uint8_t r = (b >> 3) & 0x7;
        DIS { printf("INC %s\n", REG8N(r)); }
        uint8_t v = REG8(cpu, r) + 1;
        SREG8(cpu, r, v);

        cpu->fz = v == 0;
        cpu->fn = 0;
        cpu->fh = v == 0x10;
    } else if ((b & 0xc7) == 0x05) {
        // DEC r
        uint8_t r = (b >> 3) & 0x7;
        DIS { printf("DEC %s\n", REG8N(r)); }
        uint8_t v = REG8(cpu, r) - 1;
        SREG8(cpu, r, v);

        cpu->fz = v == 0;
        cpu->fn = 1;
        cpu->fh = (v & 0xf) == 0xf;
    } else if ((b & 0xcf) == 0x03) {
        // INC ss
        uint8_t r = (b >> 4) & 0x3;
        DIS { printf("INC %s\n", REG16N(r)); }
        uint16_t v = REG16(cpu, r) + 1;
        SREG16(cpu, r, v);

        // no flags set (!)
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
    } else if (b == 0xcb) {
        b = GET8(cpu, cpu->pc++);
        if ((b & 0xf8) == 0x10) {
            // RL r
            uint8_t r = b & 0x3;
            DIS { printf("RL %s\n", REG8N(r)); }
            uint8_t v = REG8(cpu, r),
                    old_fc = cpu->fc;

            cpu->f = 0;
            cpu->fc = (v & 0x80) == 0x80;

            v = ((v & 0x7f) << 1) | old_fc;
            SREG8(cpu, r, v);

            cpu->fz = v == 0;
        } else if ((b & 0xc0) == 0x40) {
            // BIT b,r
            uint8_t bit = (b >> 3) & 0x7,
                    r = b & 0x7;
            DIS { printf("BIT %d,%s\n", bit, REG8N(r)); }

            cpu->fz = ((REG8(cpu, r) >> bit) & 0x1) == 0;
            cpu->fn = 0;
            cpu->fh = 1;
        } else if ((b & 0xc0) == 0x80) {
            // RES b,r
            uint8_t bit = (b >> 3) & 0x7,
                    r = b & 0x7;
            DIS { printf("RES %d,%s\n", bit, REG8N(r)); }
            SREG8(cpu, r, REG8(cpu, r) & ~(1 << bit));
        } else {
            fprintf(stderr, "unknown cb opcode: %x\n", b);
            dump(cpu);
            return 1;
        }
    } else if (b == 0xc3) {
        // JP nn
        uint16_t v = GET8(cpu, cpu->pc++);
        v |= GET8(cpu, cpu->pc++) << 8;
        DIS { printf("JP $%04x\n", v); }
        cpu->pc = v;
    } else if (b == 0x18) {
        // JR e
        int16_t e = (int16_t) ((int8_t) GET8(cpu, cpu->pc++)) + 2;
        DIS { printf("JR %d\n", e); }
        cpu->pc += (-2) + e;
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
    } else if (b == 0xcd) {
        // CALL nn
        uint16_t v = GET8(cpu, cpu->pc++);
        v |= GET8(cpu, cpu->pc++) << 8;
        DIS { printf("CALL $%04x\n", v); }
        PUSH16(cpu, cpu->pc);
        cpu->pc = v;

        // no flags set
    } else if (b == 0xc9) {
        // RET
        DIS { printf("RET\n"); }
        cpu->pc = POP16(cpu);
    } else if (b == 0x00) {
        // NOP
        DIS { printf("NOP\n"); }
    } else if (b == 0xf3) {
        // DI
        DIS { printf("DI\n"); }
    } else {
        fprintf(stderr, "unknown opcode: %x\n", b);
        dump(cpu);
        return 1;
    }

    return 0;
}

// vim: set sw=4 et:
