#include "main.h"
#include "proto.h"
#include "node.h"
#include "bin.h"
#include "txt.h"
#include "swap.h"
#include "option.h"

int bin2txt_main(void)
{
    node_t *n;

    demo_t *ifp = open_demo(cmd_argc > 1 ? cmd_argv[1] : NULL, "rb");
    demo_t *ofp = open_demo(cmd_argc > 2 ? cmd_argv[2] : NULL, "w");

    while ((n = read_demo(ifp))) {
        write_demo(ofp, n);
        free_nodes(n);
    }

    close_demo(ifp);
    close_demo(ofp);

    return 0;
}

int txt2bin_main(void)
{
    node_t *n;

    demo_t *ifp = open_demo(cmd_argc > 1 ? cmd_argv[1] : NULL, "r");
    demo_t *ofp = open_demo(cmd_argc > 2 ? cmd_argv[2] : NULL, "wb");

    while ((n = read_demo(ifp))) {
        write_demo(ofp, n);
        free_nodes(n);
    }

    close_demo(ifp);
    close_demo(ofp);

    return 0;
}
