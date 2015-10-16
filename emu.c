#include <stdio.h>
#include <stdlib.h>

int read_file(char const *filename, char **out, long *len) {
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
    char *rom, *cart;
    long romlen, cartlen;
    read_file("DMG_ROM.bin", &rom, &romlen);
    read_file("red.gb", &cart, &cartlen);

    if (romlen != 256) {
        fprintf(stderr, "ROM not 256 bytes; aborting\n");
        return 1;
    }

    return 0;
}

// vim: set sw=4 et:
