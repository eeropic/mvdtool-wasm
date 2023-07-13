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
    node_t *n;
    bool got_gamestate;
    demo_t *ifp, *ofp;
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

    ifp = open_demo(cmd_argv[1], "rb");

    ofp = NULL;
    blocknum = 0;
    while ((n = read_demo(ifp))) {
        got_gamestate = update_world(&world, n);
        if (!blocknum && !got_gamestate) {
            fatal("no gamestate in the first block");
        }
        if (blocknum == sp->start) {
            node_t *s;

            ofp = open_demo(sp->filename, "wb");
            if (!got_gamestate) {
                s = world_delta(NULL, &world);
                write_demo(ofp, s);
                free_nodes(s);
            }
            write_demo(ofp, n);
        } else if (blocknum > sp->start) {
            write_demo(ofp, n);
        }
        free_nodes(n);

        if (blocknum == sp->end) {
            close_demo(ofp);
            ofp = NULL;
            sp = sp->next;
            if (!sp) {
                break;
            }
        }

        blocknum++;
    }
    close_demo(ifp);
    close_demo(ofp);

    return 0;
}

