#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include "cpu.h"

int run(cpu_t *cpu, SDL_Window *window);

#define SCRW 160
#define SCRH 144
#define SCRSCALE 3

GLubyte palette[4][3] = {
    /*
    { 255, 255, 255 },
    { 254, 173, 106 },
    { 130, 50, 11 },
    { 0, 0, 0 },
    */
    { 157, 187, 41 },
    { 140, 171, 37 },
    { 50, 97, 51 },
    { 17, 55, 18 },
};

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

    int retval = run(&cpu, window);

    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return retval;
}

void lcdc_step(cpu_t *cpu, SDL_Window *window, int t);

int total_vblanks = 0;
Uint32 start_ticks = 0;

int run(cpu_t *cpu, SDL_Window *window) {
    dump(cpu);

    printf("starting execution\n");

    int running = 1;
    int retval = 0;
    int elapsed = 0;
    int last_elapsed = 0;
    start_ticks = SDL_GetTicks();

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

        int t = step(cpu);
        if (t == -1) {
            running = 0;
            retval = 1;
        } else {
            elapsed += t;
        }

        lcdc_step(cpu, window, t);

        if (elapsed > last_elapsed + 100000) {
            last_elapsed = elapsed;
            double secs = ((double) elapsed) / 4194300;
            double real_elapsed = (double) (SDL_GetTicks() - start_ticks) / 1000;
            printf("elapsed: %.02f (%0.2f vblank/sec) (real time: %.02f) (%.01f%%)\n", secs, (double) total_vblanks / secs, real_elapsed, secs / real_elapsed * 100.0);
        }
    }

    if (retval == 0) {
        dump(cpu);
    }

    return retval;
}

void lcdc_step(cpu_t *cpu, SDL_Window *window, int t) {
    cpu->lcdc_modeclock += t;

    if (cpu->lcdc_mode == 0 && cpu->lcdc_modeclock >= 204) {
        // hblank
        //
        cpu->lcdc_modeclock = 0;
        ++cpu->lcdc_line;

        if (cpu->lcdc_line == 143) {
            cpu->lcdc_mode = 1;  // vblank
            ++total_vblanks;

            glClearColor(
                ((float) palette[0][0]) / 255.0f,
                ((float) palette[0][1]) / 255.0f,
                ((float) palette[0][2]) / 255.0f,
                1.0f);

            if (!(cpu->lcdc & LCDC_OPERATE)) {
                glClear(GL_COLOR_BUFFER_BIT);
            }

            SDL_GL_SwapWindow(window);

            glClear(GL_COLOR_BUFFER_BIT);

            glViewport(0, 0, SCRW * SCRSCALE, SCRH * SCRSCALE);

            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, SCRW, SCRH, 0, -1, 1);

            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
        } else {
            cpu->lcdc_mode = 2;  // OAM read
        }
    } else if (cpu->lcdc_mode == 1 && cpu->lcdc_modeclock >= 456) {
        // vblank
        //
        cpu->lcdc_modeclock = 0;
        ++cpu->lcdc_line;

        if (cpu->lcdc_line > 153) {
            cpu->lcdc_mode = 2;  // OAM read
            cpu->lcdc_line = 0;
        }
    } else if (cpu->lcdc_mode == 2 && cpu->lcdc_modeclock >= 80) {
        // OAM read
        //
        cpu->lcdc_modeclock = 0;
        cpu->lcdc_mode = 3;  // VRAM read
    } else if (cpu->lcdc_mode == 3 && cpu->lcdc_modeclock >= 172) {
        // VRAM read
        //
        cpu->lcdc_modeclock = 0;
        cpu->lcdc_mode = 0;  // hblank

        // render scanline
        
        if (cpu->lcdc & LCDC_BG_ON) {
            int offs = (cpu->lcdc & LCDC_BG_AREA) ? 0x9C00 : 0x9800;
            offs += (((cpu->lcdc_line + cpu->lcdc_scy) & 0xff) >> 3) * 32;
            int loffs = cpu->lcdc_scx >> 3;
            int y = (cpu->lcdc_line + cpu->lcdc_scy) & 0x7;
            int x = cpu->lcdc_scx & 0x7;

            int16_t tile = (int8_t) cpu->ram[offs + loffs];
            //if ((cpu->lcdc & LCDC_BG_CHAR) && tile < 128) {
                //tile += 256;
            //}

            glBegin(GL_QUADS);
            for (int i = 0; i < 160; ++i) {
                int base = 0x8000 + (tile << 4) + (y << 1);
                int sx = 1 << (7 - x);
                int ci =
                    (cpu->ram[base] & sx) ? 1 : 0 +
                    (cpu->ram[base + 1] & sx) ? 2 : 0;
                ci = (cpu->lcdc_bgp >> (ci * 2)) & 0x3;

                glColor3ubv(palette[ci]);

                glVertex2i(i, cpu->lcdc_line);
                glVertex2i(i + 1, cpu->lcdc_line);
                glVertex2i(i + 1, cpu->lcdc_line + 1);
                glVertex2i(i, cpu->lcdc_line + 1);

                ++x;
                if (x == 8) {
                    x = 0;
                    loffs = (loffs + 1) & 0x1f;
                    tile = cpu->ram[offs + loffs];
                    //if ((cpu->lcdc & LCDC_BG_CHAR) && tile < 128) {
                        //tile += 256;
                    //}
                }
            }
            glEnd();
        }

        /*
        if (cpu->lcdc & LCDC_OBJ_ON) {
            for (int i = 0; i < 40; ++i) {
                struct {
                    uint8_t y;
                    uint8_t x;
                    uint8_t tile;
                    struct {
                        unsigned int palette : 1;
                        unsigned int xflip : 1;
                        unsigned int yflip : 1;
                        unsigned int prio : 1;
                        unsigned int unused : 4;
                    };
                } *objdata = (void *)&cpu->ram[0xfe00 + i * 4];
                int y = objdata->y - 16,
                    x = objdata->x - 8;
                if (y >= 0) {
                    printf("%d y = %d x = %d\n", i, y, x);
                }
                if (y <= cpu->lcdc_line && (y + 8) > cpu->lcdc_line) {
                    printf("HIT %d\n", i);
                }
            }
        }
        */
    }
}

// vim: set sw=4 et:
