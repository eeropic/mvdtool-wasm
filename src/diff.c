#include "main.h"
#include "swap.h"
#include "proto.h"
#include "node.h"
#include "bin.h"
#include "txt.h"
#include "option.h"

int diff_main(void)
{
    uint32_t magic;
    unsigned blocknum;
    uint8_t buf[MAX_MSGLEN], *data;
    size_t msglen1, msglen2;
    size_t i;

    FILE *ifp1, *ifp2;
    FILE *ofp = stdout;

    if (cmd_argc < 3) {
        return 1;
    }
    ifp1 = fopen(cmd_argv[1], "rb");
    if (!ifp1) {
        perror("fopen");
        return 1;
    }
    ifp2 = fopen(cmd_argv[2], "rb");
    if (!ifp2) {
        perror("fopen");
        return 1;
    }
    if (cmd_argc > 3) {
        ofp = fopen(cmd_argv[3], "w");
        if (!ofp) {
            perror("fopen");
            return 1;
        }
    }

    read_raw(&magic, sizeof(magic), ifp1);
    if (magic != MVD_MAGIC) {
        fatal("not a MVD2 file");
    }
    read_raw(&magic, sizeof(magic), ifp2);
    if (magic != MVD_MAGIC) {
        fatal("not a MVD2 file");
    }

    blocknum = 0;
    while (!feof(ifp1)) {
        data = load_bin(ifp1, &msglen1);
        if (!data) {
            data = load_bin(ifp2, &msglen2);
            if (data) {
                fprintf(stderr, "block %u: files differ in block count\n", blocknum);
            }
            break;
        }

        memcpy(buf, data, msglen1);

        data = load_bin(ifp2, &msglen2);
        if (!data) {
            fprintf(stderr, "block %u: files differ in block count\n", blocknum);
            break;
        }

        if (msglen1 != msglen2) {
            fprintf(stderr, "block %u: different length: %u != %u\n",
                    blocknum, (unsigned)msglen1, (unsigned)msglen2);
            break;
        }

        for (i = 0; i < msglen1; i++) {
            if (buf[i] != data[i]) {
                fprintf(stderr, "block %u: files differ at pos %d\n", blocknum, (unsigned)i);
                break;
            }
        }
        blocknum++;

    }

    return 0;
}

