#include "main.h"
#include "proto.h"
#include "node.h"
#include "world.h"
#include "lib.h"

static unsigned counter;
static world_state_t *ow, *cw;

//
// delta writing
//

static player_t *_build_player(const player_state_t *f, const player_state_t *t)
{
    static const player_state_t null;
    unsigned bits, statbits;
    player_t *d;
    int i;

    if (!f) f = &null;

    bits = 0;
#define DF(k,b,x) if(CMP##k(t->x,f->x)) bits|=b
    F_PLAYER

    statbits = 0;
    for (i = 0; i < MAX_STATS; i++) {
        if (t->stats[i] != f->stats[i]) {
            statbits |= 1U << i;
        }
    }
    if (statbits) {
        bits |= P_STATS;
    }

    if (!bits) {
        return NULL; // nothing to emit
    }

    d = alloc_node(NODE_PLAYER, sizeof(*d));
    d->bits = bits;
    d->statbits = statbits;
    d->s = *t;

    return d;
}

static entity_t *_build_entity(const entity_state_t *f, const entity_state_t *t)
{
    static const entity_state_t null;
    unsigned bits;
    entity_t *d;

    if (!f) f = &null;

    bits = 0;
    F_ENTITY
#undef DF

    if (!bits) {
        return NULL; // nothing to emit
    }

    d = alloc_node(NODE_ENTITY, sizeof(*d));
    d->bits = bits;
    d->s = *t;

    return d;
}

static player_t *build_player(world_player_t *cur)
{
    world_player_t *old = ow ? &ow->players[counter] : NULL;
    player_t *p = _build_player(old ? &old->s : NULL, &cur->s);

    if (!p) {
        if (old && !old->remove && !cur->remove) {
            return NULL;
        }
        p = alloc_node(NODE_PLAYER, sizeof(*p));
        p->bits = 0;
    }

    p->bits |= cur->remove;
    p->number = counter;
    return p;
}

static node_t *build_next_player(void)
{
    for (; counter < MAX_CLIENTS; counter++) {
        world_player_t *cur = &cw->players[counter];
        if (cur->active) {
            player_t *p = build_player(cur);
            if (p) {
                counter++;
                return NODE(p);
            }
        }
    }
    return NULL;
}

static entity_t *build_entity(world_entity_t *cur)
{
    world_entity_t *old = ow ? &ow->entities[counter] : NULL;
    entity_t *e = _build_entity(old ? &old->s : NULL, &cur->s);

    if (!e) {
        if (old && !old->remove && !cur->remove) {
            return NULL;
        }
        e = alloc_node(NODE_ENTITY, sizeof(*e));
        e->bits = 0;
    }

    e->bits |= cur->remove;
    e->number = counter;
    return e;
}

static node_t *build_next_entity(void)
{
    for (; counter < MAX_EDICTS; counter++) {
        world_entity_t *cur = &cw->entities[counter];
        if (cur->active) {
            entity_t *e = build_entity(cur);
            if (e) {
                counter++;
                return NODE(e);
            }
        }
    }
    return NULL;
}

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
    counter = 0;
    f->players = build_list(build_next_player);
    counter = 1;
    f->entities = build_list(build_next_entity);

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
    node_t *n, *ret, **next_p;
    char *c;
    int i;

    next_p = &ret;
    for (i = 0; i < MAX_CONFIGSTRINGS; i++) {
        c = cw->configstrings[i];
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

static void update_player(player_state_t *s, const player_t *d)
{
    int i;

#define DF(k,b,x) if(d->bits&b) CPY##k(s->x,d->s.x)
    F_PLAYER

    if (d->bits & P_STATS) {
        for (i = 0; i < MAX_STATS; i++) {
            if (d->statbits & (1U << i)) {
                s->stats[i] = d->s.stats[i];
            }
        }
    }
}

static void update_entity(entity_state_t *s, const entity_t *d)
{
    F_ENTITY
#undef DF
}


static void parse_player(player_t *p)
{
    world_player_t *w = &cw->players[p->number];

    update_player(&w->s, p);
    w->remove = p->bits & P_REMOVE;
    w->active = true;
}

static void parse_entity(entity_t *e)
{
    world_entity_t *w = &cw->entities[e->number];

    update_entity(&w->s, e);
    w->remove = e->bits & E_REMOVE;
    w->active = true;
}

static void parse_portalbits(blob_t *b)
{
    if (b) {
        memcpy(cw->portalbits, b->data, b->len);
        cw->portalbytes = b->len;
    } else {
        cw->portalbytes = 0;
    }
}

static void parse_frame(frame_t *f)
{
    parse_portalbits((blob_t *)f->portalbits);
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

