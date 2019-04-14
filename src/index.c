#include "main.h"
#include "proto.h"
#include "node.h"
#include "bin.h"
#include "swap.h"
#include "option.h"
#include "world.h"

int index_main(void)
{
    uint32_t magic;
    node_t *n, *s;
    bool got_gamestate;
    FILE *ifp, *ofp;
    unsigned blocknum;
    world_state_t world, old_world, older_world, oldest_world;

    ifp = stdin;
    ofp = stdout;

    if (cmd_argc > 1) {
        if (strcmp(cmd_argv[1], "-")) {
            ifp = fopen(cmd_argv[1], "rb");
            if (!ifp) {
                perror("fopen");
                return 1;
            }
        }
        if (cmd_argc > 2) {
            if (strcmp(cmd_argv[2], "-")) {
                ofp = fopen(cmd_argv[2], "wb");
                if (!ofp) {
                    perror("fopen");
                    return 1;
                }
            }
        }
    }

    read_raw(&magic, sizeof(magic), ifp);
    if (magic != le32(MVD_MAGIC)) {
        fatal("not a MVD2 file");
    }

    write_raw(&magic, sizeof(magic), ofp);

    blocknum = 0;
    got_gamestate = false;
    while (!feof(ifp)) {
        n = read_bin(ifp);
        if (!n) {
            break;
        }
        got_gamestate = update_world(&world, n);
        if (!blocknum && !got_gamestate) {
            fatal("no gamestate in the first block");
        }
        if (got_gamestate) {
            oldest_world = world;
        }

        if ((blocknum % 1000) == 0) {
            s = world_delta(&oldest_world, &world);
            write_bin(ofp, s);
            free_nodes(s);
            old_world = world;
            older_world = world;
            oldest_world = world;
        } else if ((blocknum % 100) == 0) {
            s = world_delta(&older_world, &world);
            write_bin(ofp, s);
            free_nodes(s);
            old_world = world;
            older_world = world;
        } else if ((blocknum % 10) == 0) {
            s = world_delta(&old_world, &world);
            write_bin(ofp, s);
            free_nodes(s);
            old_world = world;
        }

        free_nodes(n);

        blocknum++;
    }
    fclose(ifp);

    write_bin(ofp, NULL);
    fclose(ofp);

    return 0;
}

