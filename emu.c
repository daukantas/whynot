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
    { 17, 55, 18 },
    { 50, 97, 51 },
    { 140, 171, 37 },
    { 157, 187, 41 },
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

int run(cpu_t *cpu, SDL_Window *window) {
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

        int t = step(cpu);
        if (t == -1) {
            running = 0;
            retval = 1;
        }

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
        glColor3ubv(palette[3]);

        glBegin(GL_TRIANGLE_STRIP);
        glVertex2i(0, 0);
        glVertex2i(SCRW, 0);
        glVertex2i(0, SCRH);
        glVertex2i(SCRW, SCRH);
        glEnd();

        SDL_GL_SwapWindow(window);
    }
    return retval;
}

// vim: set sw=4 et:
