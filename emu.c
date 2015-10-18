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

            SDL_GL_SwapWindow(window);

            glClearColor(
                ((float) palette[0][0]) / 255.0f,
                ((float) palette[0][1]) / 255.0f,
                ((float) palette[0][2]) / 255.0f,
                1.0f);

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

        glColor3ubv(palette[3]);

        glBegin(GL_TRIANGLE_STRIP);
        glVertex2f(0, cpu->lcdc_line - 0.5f);
        glVertex2f(SCRW, cpu->lcdc_line - 0.5f);
        glVertex2f(0, cpu->lcdc_line + 0.5f);
        glVertex2f(SCRW, cpu->lcdc_line + 0.5f);
        glEnd();
    }
}

// vim: set sw=4 et:
