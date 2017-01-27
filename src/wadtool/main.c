
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *src_dir = NULL;
static const char *wad_path = NULL;

static size_t lumpcnt = 0;
static FILE *wad = NULL;

typedef struct lump {
    const char *file;
    char name[8];
    int size;

    struct lump *next;
} lump_t;

static lump_t *lumps_begin = NULL;
static lump_t *lumps_end = NULL;
static int num_lumps = 0;

static void add_marker(const char *name)
{
    lump_t *new_lump;

    new_lump = calloc(1, sizeof(*new_lump));

    strncpy(new_lump->name, name, 8);

    if (lumps_end) {
        lumps_end->next = new_lump;
        lumps_end = new_lump;
    } else {
        new_lump->next = NULL;
        lumps_begin = lumps_end = new_lump;
    }

    num_lumps++;
}

static void add_file(const char *name, const char *file)
{
    FILE *f;
    lump_t *new_lump;
    char new_path[512];

    snprintf(new_path, 512, "%s/%s", src_dir, file);
    new_lump = calloc(1, sizeof(*new_lump));

    if (!(f=fopen(new_path, "rb"))) {
        perror("Couldn't open file");
        exit(EXIT_FAILURE);
    }

    fseek(f, 0, SEEK_END);
    new_lump->size = ftell(f);
    fclose(f);

    strncpy(new_lump->name, name, 8);

    new_lump->file = file;
    if (lumps_end) {
        lumps_end->next = new_lump;
        lumps_end = new_lump;
    } else {
        new_lump->next = NULL;
        lumps_begin = lumps_end = new_lump;
    }

    num_lumps++;
}

int main(int argc, char *argv[])
{
    int filepos;
    int tableofs;
    lump_t *plump;

    if (argc != 3) {
        printf("Syntax: wadtool [source directory] [destination wad]\n");
        return EXIT_FAILURE;
    }

    src_dir = argv[1];
    wad_path = argv[2];

    if (!(wad = fopen(wad_path, "wb"))) {
        perror("Couldn't open file for writing");
        return EXIT_FAILURE;
    }

    add_marker("S_START");
    add_marker("S_END");

    add_file("PALPLAY3", "PALPLAY3.ACT");

    add_marker("G_START");
    add_file("FANCRED", "FANCRED.PNG");
    add_file("CRSHAIRS", "CRSHAIRS.PNG");
    add_file("BUTTONS", "BUTTONS.PNG");
    add_file("CONFONT", "CONFONT.PNG");
    add_file("CURSOR", "CURSOR.PNG");
    add_marker("G_END");

    add_file("MAPINFO", "MAPINFO.TXT");
    add_file("ANIMDEFS", "ANIMDEFS.TXT");
    add_file("SKYDEFS", "SKYDEFS.TXT");

    add_marker("ENDOFWAD");

    filepos = 12 + 16 * num_lumps;
    tableofs = 12;

    fwrite("PWAD", 1, 4, wad);
    fwrite(&num_lumps, sizeof(num_lumps), 1, wad);
    fwrite(&tableofs, sizeof(tableofs), 1, wad);

    plump = lumps_begin;
    while (plump) {
        printf("Writing lump header %.8s with offset %d\n", plump->name, filepos);

        fwrite(&filepos, sizeof(filepos), 1, wad);
        fwrite(&plump->size, sizeof(plump->size), 1, wad);
        fwrite(plump->name, 1, 8, wad);

        filepos += plump->size;
        plump = plump->next;
    }

    plump = lumps_begin;
    while (plump) {
        FILE *f;
        char *buf, path[512];

        if (plump->size == 0) {
            plump = plump->next;
            continue;
        }

        printf("Writing lump %.8s with size %d at %lu\n", plump->name, plump->size, ftell(wad));

        snprintf(path, 512, "%s/%s", src_dir, plump->file);
        if (!(f = fopen(path, "rb"))) {
            perror("Couldn't open file for reading");
            return EXIT_FAILURE;
        }
        buf = malloc(plump->size);
        fread(buf, 1, plump->size, f);
        fclose(f);
        if (fwrite(buf, 1, plump->size, wad) != plump->size) {
            printf("Couldn't write.\n");
            return EXIT_FAILURE;
        }
        free(buf);

        plump = plump->next;
    }

    fclose(wad);

    return EXIT_SUCCESS;
}
