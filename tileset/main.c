#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>
#include <SDL_opengl.h>

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
    uint8_t *dump;
    long dumplen;
    read_file(argv[1], &dump, &dumplen);

    if (dumplen != 0x10000) {
        fprintf(stderr, "dump not 0x10000 bytes; aborting\n");
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "tileset",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        640, 480,
        SDL_WINDOW_OPENGL);

    if (!window) {
        fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glcontext = SDL_GL_CreateContext(window);

    int running = 1;

    union {
        uint8_t raw;
        struct {
            unsigned int c0 : 2;
            unsigned int c1 : 2;
            unsigned int c2 : 2;
            unsigned int c3 : 2;
        };
    } map = { .raw = dump[0xff47] };
    int pmap[4] = { map.c0, map.c1, map.c2, map.c3 };

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = 0;
                    break;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);

        glViewport(0, 0, 640, 480);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, 640, 480, 0, -1, 1);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        for (int i = 0; i < 0x20; ++i) {
            glPushMatrix();
            glTranslatef(10, 10 + i * 10, 0);

            for (int y = 0; y < 8; ++y) {
                int offset = 0x8000 + i * 0x10 + y * 2;
                for (int x = 0; x < 8; ++x) {
                    int sx = 1 << (7 - x);
                    int p =
                        (dump[offset] & sx) ? 1 : 0 +
                        (dump[offset + 1] & sx) ? 2 : 0;
                    glColor3ubv(palette[pmap[p]]);
                    glBegin(GL_QUADS);
                    glVertex2i(x, y);
                    glVertex2i(x + 1, y);
                    glVertex2i(x + 1, y + 1);
                    glVertex2i(x, y + 1);
                    glEnd();
                }
            }

            glPopMatrix();
        }

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

// vim: set sw=4 et:
