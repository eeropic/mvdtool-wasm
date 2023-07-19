#include "main.h"
#include "proto.h"
#include "node.h"
#include "bin.h"
#include "swap.h"
#include "option.h"
#include "lib.h"

static demo_t *ifp;

static unsigned blocknum, clientnum;
static int what = -1;
static bool newline = true;

static size_t format_time(char *buf, size_t size)
{
    return snprintf(buf, size, "[%u:%02u.%u] ",
                    blocknum / 600,
                    (blocknum / 10) % 60,
                    blocknum % 10);
}

static void print(const char *s)
{
    char text[2048];
    char buf[MAX_QPATH];
    char *p, *maxp;
    size_t len;
    int c;

    len = format_time(buf, sizeof(buf));

    p = text;
    maxp = text + sizeof(text) - 1;
    while (*s) {
        if (newline) {
            if (len > 0 && p + len < maxp) {
                memcpy(p, buf, len);
                p += len;
            }
            newline = false;
        }

        if (p == maxp) {
            break;
        }

        c = *s++;
        if (c == '\n') {
            newline = true;
        } else {
            c = Q_charascii(c);
            if (c < 32) {
                c = '.';
            }
        }

        *p++ = c;
    }
    *p = 0;

    len = p - text;
    fwrite(text, len, 1, stdout);
}

static void parse_gamestate(game_state_t *g)
{
    node_t *n;

    for (n = g->configstrings; n; n = n->next) {
        if (((string_t *)n)->index == CS_NAME) {
            print("-----------------------\n");
            print(((string_t *)n)->data);
            print("\n");
            break;
        }
    }

    clientnum = g->clientnum;
}

static void parse_serverdata(serverdata_t *s)
{
    print("-----------------------\n");
    print(s->levelname);
    print("\n");
}

static void parse_print(string_t *s)
{
    if (what & (1 << s->index)) {
        print(s->data);
    }
}

static void parse_centerprint(string_t *s)
{
    print(s->data);
    print("\n");
}

static void parse_unicast(unicast_t *u)
{
    void *n;

    if (u->clientnum != clientnum) {
        return;
    }

    for (n = u->data; n; n = ((node_t *)n)->next) {
        switch (((node_t *)n)->type) {
        case NODE_PRINT:
            parse_print(n);
            break;
        case NODE_CENTERPRINT:
            parse_centerprint(n);
            break;
        default:
            break;
        }
    }
}

static void parse_multicast(multicast_t *m)
{
    void *n;

    for (n = m->data; n; n = ((node_t *)n)->next) {
        switch (((node_t *)n)->type) {
        case NODE_PRINT:
            parse_print(n);
            break;
        case NODE_CENTERPRINT:
            parse_centerprint(n);
            break;
        default:
            break;
        }
    }
}

static void parse_message(void *n)
{
    for (; n; n = ((node_t *)n)->next) {
        switch (((node_t *)n)->type) {
        case NODE_GAMESTATE:
            parse_gamestate(n);
            break;
        case NODE_SERVERDATA:
            parse_serverdata(n);
            break;
        case NODE_CONFIGSTRING:
            //parse_configstring( n );
            break;
        case NODE_PRINT:
            parse_print(n);
            break;
        case NODE_CENTERPRINT:
            parse_centerprint(n);
            break;
        case NODE_UNICAST:
            parse_unicast(n);
            break;
        case NODE_MULTICAST:
            parse_multicast(n);
            break;
        default:
            break;
        }
    }
}

int strings_main(void)
{
    node_t *n;

    ifp = open_demo(cmd_argc > 1 ? cmd_argv[1] : NULL, MODE_READ);

    while ((n = read_demo(ifp))) {
        parse_message(n);
        free_nodes(n);
        blocknum++;
    }
    close_demo(ifp);

    return 0;
}
