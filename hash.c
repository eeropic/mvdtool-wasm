#include "main.h"
#include "proto.h"
#include "node.h"
#include "bin.h"
#include "swap.h"
#include "sha1.h"
#include "option.h"

int hash_main(void)
{
    uint32_t magic;
    size_t msglen;
    uint8_t *data;
    SHA1_CTX ctx;
    uint8_t digest[20];
    int i;

    FILE *ifp = stdin;
    FILE *ofp = stdout;

    if (cmd_argc > 1) {
        ifp = fopen(cmd_argv[1], "rb");
        if (!ifp) {
            perror("fopen");
            return 1;
        }
        if (cmd_argc > 2) {
            ofp = fopen(cmd_argv[2], "w");
            if (!ofp) {
                perror("fopen");
                return 1;
            }
        }
    }

    read_raw(&magic, sizeof(magic), ifp);
    if (magic != MVD_MAGIC) {
        fatal("not a MVD2 file");
    }

    while (!feof(ifp)) {
        data = load_bin(ifp, &msglen);
        if (!data) {
            break;
        }

        sha1_init(&ctx);
        sha1_update(&ctx, data, msglen);
        sha1_final(&ctx, digest);

        for (i = 0; i < 20; i++) {
            fprintf(ofp, "%02x", digest[i]);
        }
        fprintf(ofp, "\n");
    }

    return 0;
}

