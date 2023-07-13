#include "main.h"
#include "proto.h"
#include "node.h"
#include "bin.h"
#include "swap.h"
#include "option.h"
#include "world.h"

typedef struct cut_point_s {
    struct cut_point_s *next;
    unsigned start, end;
} cut_point_t;

static cut_point_t *parse_cut_points(void)
{
    cut_point_t *cp, *ret, **next_p;
    unsigned start, end, prevnum = 0;
    char *s, *p;
    int i;

    cp = NULL;
    next_p = &ret;
    for (i = 3; i < cmd_argc; i++) {
        s = cmd_argv[i];
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
        if (start == prevnum && cp) {
            cp->end = end;
            prevnum = end;
            continue;
        }


        cp = malloc(sizeof(*cp));
        cp->start = start;
        cp->end = end;

        prevnum = end;
        *next_p = cp;
        next_p = &cp->next;
        continue;

bad:
        fprintf(stderr, "Ignoring malformed cut point at pos %d\n", i - 1);
    }
    *next_p = NULL;

    return ret;
}

// append n to s, removing any frames and configstrings from n
static node_t *hijack_block(node_t *s, node_t *n)
{
    node_t *next, **next_p;

    s->next = n;

    next_p = &s->next;
    for (; n; n = next) {
        next = n->next;
        switch (n->type) {
        case NODE_CONFIGSTRING:
        case NODE_FRAME:
            free_node(n);
            *next_p = next;
            //fprintf(stderr,"removed node %d\n",n->type);
            break;
        default:
            next_p = &n->next;
            break;
        }

    }
    return s;
}

int cut_main(void)
{
    cut_point_t *cp;
    node_t *n, *s;
    bool got_gamestate;
    demo_t *ifp, *ofp;
    unsigned blocknum;
    world_state_t world, old_world;

    if (cmd_argc < 3) {
        fprintf(stderr, "Usage: %s <infile> <outfile> [start,end] [...]\n", cmd_argv[0]);
        return 1;
    }

    cp = parse_cut_points();
    if (!cp) {
        fprintf(stderr, "No valid cut points to process\n");
        return 1;
    }

    ifp = open_demo(cmd_argv[1], "rb");
    ofp = open_demo(cmd_argv[2], "wb");

    blocknum = 0;
    got_gamestate = false;
    while ((n = read_demo(ifp))) {
        got_gamestate |= update_world(&world, n);
        if (!blocknum && !got_gamestate) {
            fatal("no gamestate in the first block");
        }
        if (cp) {
            if (blocknum < cp->start) {
                write_demo(ofp, n);
                got_gamestate = false;
            } else if (blocknum == cp->start) {
                old_world = world;
            } else if (blocknum == cp->end + 1) {
                if (got_gamestate) {
                    // if we got level change in between, write gamestate first,
                    // then entire current frame
                    s = world_delta(NULL, &world);
                    write_demo(ofp, s);
                    free_nodes(s);
                    got_gamestate = false;
                } else {
                    // generate delta frame and configstrings and insert them
                    // into current frame, resulting in seamless stitch
                    s = world_delta(&old_world, &world);
                    n = hijack_block(s, n);
                }
                write_demo(ofp, n);
                cp = cp->next;
            }
        } else {
            write_demo(ofp, n);
        }
        free_nodes(n);

        blocknum++;
    }
    close_demo(ifp);
    close_demo(ofp);

    return 0;
}

