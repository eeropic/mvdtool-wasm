#include "main.h"
#include "proto.h"
#include "node.h"
#include "bin.h"
#include "swap.h"
#include "sha1.h"
#include "option.h"

int hash_main(void)
{
    size_t msglen;
    uint8_t *data;
    SHA1_CTX ctx;
    uint8_t digest[20];
    int i;

    demo_t *ifp = open_demo(cmd_argc > 1 ? cmd_argv[1] : NULL, MODE_READ);

    while ((data = load_bin(ifp, &msglen))) {
        sha1_init(&ctx);
        sha1_update(&ctx, data, msglen);
        sha1_final(&ctx, digest);

        for (i = 0; i < 20; i++) {
            printf("%02x", digest[i]);
        }
        printf("\n");
    }

    return 0;
}
