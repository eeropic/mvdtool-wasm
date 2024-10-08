#include "main.h"
#include "proto.h"
#include "node.h"
#include "bin.h"
#include "txt.h"
#include "txt_json.h"
#include "swap.h"
#include "option.h"

int convert_main(unsigned imode, unsigned omode)
{
    demo_t *ifp = open_demo(cmd_argc > 1 ? cmd_argv[1] : NULL, imode);
    node_t *n = read_demo(ifp);

    omode |= ifp->mode & MODE_DM2;
    demo_t *ofp = open_demo(cmd_argc > 2 ? cmd_argv[2] : NULL, omode | MODE_WRITE);

    while (n) {
        write_demo(ofp, n);
        free_nodes(n);
        n = read_demo(ifp);
    }

    close_demo(ifp);
    close_demo(ofp);

    return 0;
}
