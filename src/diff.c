#include "main.h"
#include "swap.h"
#include "proto.h"
#include "node.h"
#include "bin.h"
#include "txt.h"
#include "option.h"

int diff_main(void)
{
    unsigned blocknum;
    uint8_t buf[MAX_MSGLEN], *data;
    size_t msglen1, msglen2;
    size_t i;
    demo_t *ifp1, *ifp2;

    if (cmd_argc < 3) {
        return 1;
    }

    ifp1 = open_demo(cmd_argv[1], MODE_READ);
    ifp2 = open_demo(cmd_argv[2], MODE_READ);

    blocknum = 0;
    while (1) {
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

        ifp1->blocknum = ifp2->blocknum = blocknum++;
    }

    return 0;
}
