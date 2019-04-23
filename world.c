#include "main.h"
#include "proto.h"
#include "node.h"
#include "world.h"
#include "lib.h"

static world_state_t *ow, *cw;

//
// delta writing
//

#define DF(k,b,x) if(CMP##k(cur->s.x,from->x)) bits|=b

static node_t *build_players(void)
{
    static const player_state_t null;
    node_t *ret, **next_p = &ret;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        world_player_t *cur = &cw->players[i];
        if (!cur->active) {
            continue;
        }

        world_player_t *old = ow ? &ow->players[i] : NULL;
        const player_state_t *from = old ? &old->s : &null;
        unsigned bits = 0, statbits = 0;

        F_PLAYER

        for (int j = 0; j < MAX_STATS; j++) {
            if (cur->s.stats[j] != from->stats[j]) {
                statbits |= 1U << j;
            }
        }
        if (statbits) {
            bits |= P_STATS;
        }

        if (!bits && old && !old->remove && !cur->remove) {
            continue;   // nothing to emit
        }

        player_t *p = alloc_node(NODE_PLAYER, sizeof(*p));
        p->bits = bits | cur->remove;
        p->number = i;
        p->statbits = statbits;
        p->s = cur->s;

        *next_p = &p->node;
        next_p = &p->node.next;
    }
    *next_p = NULL;

    return ret;
}

static node_t *build_entities(void)
{
    static const entity_state_t null;
    node_t *ret, **next_p = &ret;

    for (int i = 1; i < MAX_EDICTS; i++) {
        world_entity_t *cur = &cw->entities[i];
        if (!cur->active) {
            continue;
        }

        world_entity_t *old = ow ? &ow->entities[i] : NULL;
        const entity_state_t *from = old ? &old->s : &null;
        unsigned bits = 0;

        F_ENTITY

        if (!bits && old && !old->remove && !cur->remove) {
            continue;   // nothing to emit
        }

        entity_t *e = alloc_node(NODE_ENTITY, sizeof(*e));
        e->bits = bits | cur->remove;
        e->number = i;
        e->s = cur->s;

        *next_p = &e->node;
        next_p = &e->node.next;
    }
    *next_p = NULL;

    return ret;
}

#undef DF

static node_t *build_blob(void *data, size_t len)
{
    blob_t *b;

    if (!len) {
        return NULL;
    }

    b = alloc_node(NODE_BLOB, sizeof(*b) + len - 1);
    b->len = len;
    memcpy(b->data, data, len);

    return NODE(b);
}

static node_t *build_frame(void)
{
    frame_t *f;

    f = alloc_node(NODE_FRAME, sizeof(*f));
    f->portalbits = build_blob(cw->portalbits, cw->portalbytes);
    f->players = build_players();
    f->entities = build_entities();

    return NODE(f);
}

static node_t *build_configstring(unsigned index, const char *c)
{
    string_t *s;
    size_t len;

    len = strlen(c);
    s = alloc_node(NODE_CONFIGSTRING, sizeof(*s) + len);
    s->index = index;
    memcpy(s->data, c, len + 1);
    s->len = len;

    return NODE(s);
}

static node_t *build_configstrings(void)
{
    node_t *n, *ret, **next_p = &ret;

    for (int i = 0; i < MAX_CONFIGSTRINGS; i++) {
        char *c = cw->configstrings[i];
        if (ow) {
            if (!strcmp(c, ow->configstrings[i])) {
                continue;
            }
        } else if (!c[0]) {
            continue;
        }

        n = build_configstring(i, c);
        *next_p = n;
        next_p = &n->next;
    }
    *next_p = NULL;

    return ret;
}

static node_t *build_game_state(void)
{
    game_state_t *g;

    g = alloc_node(NODE_GAMESTATE, sizeof(*g));
    g->majorversion = PROTOCOL_VERSION_MVD;
    g->minorversion = PROTOCOL_VERSION_MVD_CURRENT;
    g->mvdflags = cw->mvdflags;
    g->servercount = cw->servercount;
    g->clientnum = cw->clientnum;
    strcpy(g->gamedir, cw->gamedir);

    g->baseframe = build_frame();
    g->configstrings = build_configstrings();

    return NODE(g);
}

// emits either complete gamestate, or regular delta
node_t *world_delta(world_state_t *from, world_state_t *to)
{
    node_t *n;

    ow = from;
    cw = to;

    if (from) {
        n = build_frame();
        n->next = build_configstrings();
    } else {
        n = build_game_state();
    }

    return n;
}

//
// delta parsing
//

static void parse_player(player_t *p)
{
    world_player_t *w = &cw->players[p->number];

#define DF(k,b,x) if(p->bits&b) CPY##k(w->s.x,p->s.x)
    F_PLAYER
#undef DF

    if (p->bits & P_STATS) {
        for (int i = 0; i < MAX_STATS; i++) {
            if (p->statbits & (1U << i)) {
                w->s.stats[i] = p->s.stats[i];
            }
        }
    }

    w->remove = p->bits & P_REMOVE;
    w->active = true;
}

static void parse_entity(entity_t *e)
{
    world_entity_t *w = &cw->entities[e->number];

#define DF(k,b,x) if(e->bits&b) CPY##k(w->s.x,e->s.x)
    F_ENTITY
#undef DF

    w->remove = e->bits & E_REMOVE;
    w->active = true;
}

static void parse_frame(frame_t *f)
{
    blob_t *b = (blob_t *)f->portalbits;

    if (b) {
        memcpy(cw->portalbits, b->data, b->len);
        cw->portalbytes = b->len;
    } else {
        cw->portalbytes = 0;
    }

    iter_list(f->players, parse_player);
    iter_list(f->entities, parse_entity);
}

static void parse_configstring(string_t *s)
{
    memcpy(cw->configstrings[s->index], s->data, s->len);
}

static void parse_gamestate(game_state_t *g)
{
    memset(cw, 0, sizeof(*cw));
    cw->mvdflags = g->mvdflags;
    cw->servercount = g->servercount;
    cw->clientnum = g->clientnum;
    strcpy(cw->gamedir, g->gamedir);
    parse_frame((frame_t *)g->baseframe);
    iter_list(g->configstrings, parse_configstring);
}

bool update_world(world_state_t *world, void *n)
{
    cw = world;

    for (; n; n = ((node_t *)n)->next) {
        switch (((node_t *)n)->type) {
        case NODE_GAMESTATE:
            parse_gamestate(n);
            return true;
        case NODE_FRAME:
            parse_frame(n);
            break;
        case NODE_CONFIGSTRING:
            parse_configstring(n);
            break;
        default:
            break;
        }
    }
    return false;
}

