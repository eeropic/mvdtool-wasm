#include "main.h"
#include "proto.h"
#include "node.h"
#include "bin.h"
#include "txt.h"
#include "txt_json.h"
#include "option.h"

void fatal(const char *fmt, ...)
{
    va_list argptr;

    va_start(argptr, fmt);
    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, fmt, argptr);
    fprintf(stderr, "\n");
    va_end(argptr);

    exit(1);
}

void read_raw(void *buf, size_t len, FILE *fp)
{
    if (fread(buf, 1, len, fp) != len) {
        if (ferror(fp)) {
            fatal("failed to read file");
        } else {
            fatal("unexpected end of file");
        }
    }
}

void write_raw(void *buf, size_t len, FILE *fp)
{
    if (fwrite(buf, 1, len, fp) != len) {
        fatal("failed to write file");
    }
}

demo_t *open_demo(const char *path, unsigned mode)
{
    demo_t *demo;
    FILE *fp;

    if (path && strcmp(path, "-")) {
        char str[3];
        if (mode & MODE_WRITE)
            strcpy(str, "w");
        else
            strcpy(str, "r");
        if (!(mode & MODE_TXT))
            strcat(str, "b");
        fp = fopen(path, str);
        if (!fp)
            fatal("couldn't open %s: %s", path, strerror(errno));
        mode |= MODE_FILE;
    } else if (mode & MODE_WRITE) {
        fp = stdout;
    } else {
        fp = stdin;
    }

    demo = calloc(1, sizeof(*demo));
    demo->mode = mode;
    demo->fp = fp;
    if (path)
        demo->path = strdup(path);
    return demo;
}

void close_demo(demo_t *demo)
{
    if (!demo)
        return;

    if ((demo->mode & MODE_WRITE) && demo->blocknum) {
        if (demo->mode & MODE_TXT) {
            write_raw("}\n", 2, demo->fp);
        } else if (demo->mode & MODE_DM2) {
            uint32_t v = -1;
            write_raw(&v, sizeof(v), demo->fp);
        } else {
            uint16_t v = 0;
            write_raw(&v, sizeof(v), demo->fp);
        }
    }

    int r = 0;
    if (demo->mode & MODE_FILE)
        r = fclose(demo->fp);
    else if (demo->mode & MODE_WRITE)
        r = fflush(demo->fp);
    if (r)
        fatal("failed to write file");

    free(demo->path);
    free(demo);
}

node_t *read_demo(demo_t *demo)
{
    node_t *n;

    if (demo->mode & MODE_EOF)
        return NULL;

    n = (demo->mode & MODE_TXT) ? read_txt(demo) : read_bin(demo);
    if (!n)
        demo->mode |= MODE_EOF;
    else
        demo->blocknum++;
    return n;
}

size_t write_demo(demo_t *demo, node_t *n)
{
    size_t r;
    if (demo->mode & MODE_JSON) {
        r = write_txt_json(demo, n);
    } else if (demo->mode & MODE_TXT) {
        r = write_txt(demo, n);
    } else {
        r = write_bin(demo, n);
    }
    demo->blocknum++;
    return r;
}

// 12345
// 12345.1
// 12:45
// 12:45.1
bool parse_timespec(const char *s, unsigned *blocknum)
{
    unsigned c1, c2, c3;
    char *p;

    c1 = strtoul(s, &p, 10);
    if (!*p) {
        *blocknum = c1; // block number
        return true;
    }
    if (*p == '.') {
        c2 = strtoul(p + 1, &p, 10);
        if (*p || c2 > 9) {
            return false;
        }
        *blocknum = c1 * 10 + c2; // sec.frac
        return true;
    }
    if (*p == ':') {
        c2 = strtoul(p + 1, &p, 10);
        if (!*p) {
            *blocknum = c1 * 600 + c2 * 10; // min:sec
            return true;
        }
        if (*p == '.') {
            c3 = strtoul(p + 1, &p, 10);
            if (*p || c3 > 9) {
                return false;
            }
            *blocknum = c1 * 600 + c2 * 10 + c3; // min:sec.frac
            return true;
        }
        return false;
    }
    return false;
}

static int help_main(void)
{
    fprintf(stderr,
            "Usage: mvdtool <mode> [options]\n"
            "Version: " MVDTOOL_VERSION "\n"
            "\n"
            "Mode     Description\n"
            "-------- ----------------------------------------------\n"
            "bin2txt  Convert demo from binary to text format\n"
            "txt2bin  Convert demo from text to binary format\n"
            "hash     Compute SHA-1 hashes of binary demo blocks\n"
            "strings  Parse demo and print messages and timestamps\n"
            "split    Split demo by timestamps into multiple files\n"
            "cut      Seamlessy remove chosen parts of demo recording\n"
            "diff     Compare demos block-by-block\n"
            "help     Show this message\n");
    return 0;
}

int main(int argc, char **argv)
{
    char *name;

    if (argc < 2) {
        goto usage;
    }

    name = argv[1];
    cmd_argc = argc - 1;
    cmd_argv = argv + 1;

    if (!strcmp(name, "bin2txt")) {
        return convert_main(0, MODE_TXT);
    }
    if (!strcmp(name, "bin2json")) {
        return convert_main(0, MODE_JSON);
    }
    if (!strcmp(name, "txt2bin")) {
        return convert_main(MODE_TXT, 0);
    }
    if (!strcmp(name, "hash")) {
        return hash_main();
    }
    if (!strcmp(name, "strings")) {
        return strings_main();
    }
    if (!strcmp(name, "split")) {
        return split_main();
    }
    if (!strcmp(name, "cut")) {
        return cut_main();
    }
    if (!strcmp(name, "diff")) {
        return diff_main();
    }
    if (!strcmp(name, "help")) {
        return help_main();
    }

usage:
    fprintf(stderr, "Usage: %s <bin2txt|bin2json|txt2bin|hash|strings|split|cut|diff|help> [options]\n", argv[0]);
    return 1;
}

