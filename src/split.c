#include "main.h"
#include "proto.h"
#include "node.h"
#include "bin.h"
#include "swap.h"
#include "option.h"
#include "world.h"

typedef struct split_point_s {
    struct split_point_s *next;
    unsigned start, end;
    char filename[1];
} split_point_t;

static split_point_t *parse_split_points(void)
{
    split_point_t *sp, *ret, **next_p;
    unsigned start, end, prevnum = 0;
    char *s, *p, *filename;
    size_t len;
    int i;

    next_p = &ret;
    for (i = 2; i < cmd_argc; i++) {
        s = cmd_argv[i];
        p = strchr(s, '@');
        if (!p) {
            goto bad;
        }

        *p++ = 0;
        filename = p;
        start = prevnum;
        end = UINT_MAX;

        p = strchr(s, ',');
        if (!p) {
            goto bad;
        }
        *p++ = 0;
        if (*s && !parse_timespec(s, &start)) {
            goto bad;
        }
        if (*p && !parse_timespec(p, &end)) {
            goto bad;
        }
        if (start < prevnum || end <= start) {
            goto bad;
        }


        len = strlen(filename);
        sp = malloc(sizeof(*sp) + len);
        memcpy(sp->filename, filename, len + 1);
        sp->start = start;
        sp->end = end;

        prevnum = end;
        *next_p = sp;
        next_p = &sp->next;
        continue;

bad:
        fprintf(stderr, "Ignoring malformed split point at pos %d\n", i - 1);
    }
    *next_p = NULL;

    return ret;
}

int split_main(void)
{
    split_point_t *sp;
    uint32_t magic;
    node_t *n;
    bool got_gamestate;
    FILE *ifp, *ofp;
    unsigned blocknum;
    world_state_t world;

    if (cmd_argc < 2) {
        fprintf(stderr, "Usage: %s <infile> [start,end@outfile1] [...]\n", cmd_argv[0]);
        return 1;
    }

    sp = parse_split_points();
    if (!sp) {
        fprintf(stderr, "No valid split points to process\n");
        return 1;
    }

    if (!strcmp(cmd_argv[1], "-")) {
        ifp = stdin;
    } else {
        ifp = fopen(cmd_argv[1], "rb");
        if (!ifp) {
            perror("fopen");
            return 1;
        }
    }

    read_raw(&magic, sizeof(magic), ifp);
    if (magic != MVD_MAGIC) {
        fatal("not a MVD2 file");
    }

    ofp = NULL;
    blocknum = 0;
    while (!feof(ifp)) {
        n = read_bin(ifp);
        if (!n) {
            break;
        }
        got_gamestate = update_world(&world, n);
        if (!blocknum && !got_gamestate) {
            fatal("no gamestate in the first block");
        }
        if (blocknum == sp->start) {
            node_t *s;

            ofp = fopen(sp->filename, "wb");
            if (!ofp) {
                perror("fopen");
                return 1;
            }
            write_raw(&magic, sizeof(magic), ofp);
            if (!got_gamestate) {
                s = world_delta(NULL, &world);
                write_bin(ofp, s);
                free_nodes(s);
            }
            write_bin(ofp, n);
        } else if (blocknum > sp->start) {
            write_bin(ofp, n);
        }
        free_nodes(n);

        if (blocknum == sp->end) {
            write_bin(ofp, NULL);
            fclose(ofp);
            ofp = NULL;
            sp = sp->next;
            if (!sp) {
                break;
            }
        }

        blocknum++;
    }
    fclose(ifp);

    if (ofp) {
        write_bin(ofp, NULL);
        fclose(ofp);
    }

    return 0;
}

