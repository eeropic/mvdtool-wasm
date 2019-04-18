#include "main.h"
#include "proto.h"
#include "node.h"
#include "txt.h"
#include "lib.h"

static FILE *ifp, *ofp;

//
// reading
//

static struct {
    char data[MAX_NET_STRING * 4];
    char token[MAX_NET_STRING];
    size_t len;
    char *ptr;
    int number;
} line;

static void read_line(void)
{
    line.number++;
    if (!fgets(line.data, sizeof(line.data), ifp)) {
        if (ferror(ifp)) {
            fatal("reading input file");
        }
        fatal("unexpected end of file");
    }
    line.ptr = line.data;
}

static inline void addchar(int c)
{
    if (line.len < sizeof(line.token) - 1) {
        line.token[line.len++] = c;
    } else {
        fatal("line %d: oversize token", line.number);
    }
}

static void parse_escape(void)
{
    int c, c1, c2;

    switch (line.ptr[1]) {
    case '\"':
        c = '\"';
        break;
    case '\\':
        c = '\\';
        break;
    case 't':
        c = '\t';
        break;
    case 'r':
        c = '\r';
        break;
    case 'n':
        c = '\n';
        break;
    case 'x':
        c1 = Q_charhex(line.ptr[2]);
        if (c1 != -1) {
            c2 = Q_charhex(line.ptr[3]);
            if (c2 != -1) {
                c = (c1 << 4) | c2;
                line.ptr += 2;
            } else {
                c = c1;
                line.ptr++;
            }
            break;
        }
        // fall through
    default:
        fatal("line %d: bad escape sequence", line.number);
    }
    addchar(c);
    line.ptr += 2;
}

static void parse_quoted(void)
{
    while (*line.ptr) {
        if (*line.ptr == '\\') {
            parse_escape();
            continue;
        }
        if (*line.ptr == '\"') {
            line.ptr++;
            break;
        }
        addchar(*line.ptr++);
    }
}

static void parse_word(void)
{
    do {
        addchar(*line.ptr++);
    } while (*line.ptr > 32);
}

static char *parse(void)
{
    line.token[0] = 0;
    line.len = 0;

    if (!line.ptr) {
        read_line();
    }

skip:
    while (*line.ptr <= 32) {
        if (!*line.ptr) {
            read_line();
            continue;
        }
        line.ptr++;
    }

    if (*line.ptr == '/' && *(line.ptr + 1) == '/') {
        read_line();
        goto skip;
    }

    if (*line.ptr == '\"') {
        line.ptr++;
        parse_quoted();
    } else {
        parse_word();
    }

    line.token[line.len] = 0;
    return line.token;
}

static void expect(const char *what)
{
    char *token = parse();

    if (strcmp(token, what)) {
        fatal("line %d: expected %s, but got %s",
              line.number, what, token);
    }
}

static void __attribute__((noreturn)) unknown(void)
{
    fatal("line %d: unknown token: %s", line.number, line.token);
}

static void __attribute__((noreturn)) undefined(const char *what)
{
    fatal("line %d: undefined token: %s", line.number, what);
}

static size_t parse_string(char *buf, size_t size)
{
    size_t len;

    parse();
    if (size) {
        len = line.len > size - 1 ? size - 1 : line.len;
        memcpy(buf, line.token, len);
        buf[len] = 0;
    }

    return line.len;
}

static int parse_int(int b_min, int b_max)
{
    long v = strtol(parse(), NULL, 0);

    if (v < b_min || v > b_max) {
        fatal("line %d: value out of range: %ld", line.number, v);
    }
    return v;
}

static unsigned parse_uint(unsigned b_min, unsigned b_max)
{
    unsigned long v = strtoul(parse(), NULL, 0);

    if (v < b_min || v > b_max) {
        fatal("line %d: value out of range: %lu", line.number, v);
    }
    return v;
}

static void parse_int_v(int *v, size_t count, int b_min, int b_max)
{
    size_t i;

    for (i = 0; i < count; i++) {
        v[i] = parse_int(b_min, b_max);
    }
}

static void parse_uint_v(unsigned *v, size_t count, unsigned b_min, unsigned b_max)
{
    size_t i;

    for (i = 0; i < count; i++) {
        v[i] = parse_uint(b_min, b_max);
    }
}

#define parse_uint8()   parse_uint(0,UINT8_MAX)
#define parse_uint16()  parse_uint(0,UINT16_MAX)
#define parse_uint32()  parse_uint(0,UINT32_MAX)
#define parse_int16()   parse_int(INT16_MIN,INT16_MAX)
#define parse_int8_v(v,count)    parse_int_v(v,count,INT8_MIN,INT8_MAX)
#define parse_int16_v(v,count)   parse_int_v(v,count,INT16_MIN,INT16_MAX)
#define parse_uint8_v(v,count)   parse_uint_v(v,count,0,UINT8_MAX)

static unsigned parse_stat(int *stats)
{
    unsigned index = MAX_STATS;
    int value = INT16_MAX + 1;
    char *tok;

    expect("{");
    while (1) {
        tok = parse();
        if (!strcmp(tok, "index")) {
            index = parse_uint(0, MAX_STATS - 1);
        } else if (!strcmp(tok, "value")) {
            value = parse_int16();
        } else if (!strcmp(tok, "}")) {
            break;
        } else {
            unknown();
        }
    }

    if (index == MAX_STATS) undefined("index");
    if (value == INT16_MAX + 1) undefined("value");

    stats[index] = value;
    return index;
}

static unsigned parse_stats(int *stats)
{
    unsigned bits = 0;
    char *tok;

    expect("{");
    while (1) {
        tok = parse();
        if (!strcmp(tok, "stat")) {
            bits |= 1U << parse_stat(stats);
        } else if (!strcmp(tok, "}")) {
            break;
        } else {
            unknown();
        }
    }

    return bits;
}

static node_t *_parse_player(void)
{
    player_t *p;
    char *tok;

    p = alloc_node(NODE_PLAYER, sizeof(*p));
    p->number = CLIENTNUM_NONE;
    p->bits = 0;
    p->statbits = 0;

    expect("{");
    while (1) {
        tok = parse();
        if (!strcmp(tok, "number")) {
            p->number = parse_uint(0, CLIENTNUM_NONE - 1);
            continue;
        }
        if (!strcmp(tok, "pm_type")) {
            p->s.pm_type = parse_uint8();
            p->bits |= P_TYPE;
            continue;
        }
        if (!strcmp(tok, "origin_xy")) {
            parse_int16_v(p->s.origin_xy, 2);
            p->bits |= P_ORIGIN;
            continue;
        }
        if (!strcmp(tok, "origin_z")) {
            p->s.origin_z = parse_int16();
            p->bits |= P_ORIGIN2;
            continue;
        }
        if (!strcmp(tok, "viewoffset")) {
            parse_int8_v(p->s.viewoffset, 3);
            p->bits |= P_VIEWOFFSET;
            continue;
        }
        if (!strcmp(tok, "viewangles_xy")) {
            parse_int16_v(p->s.viewangles_xy, 2);
            p->bits |= P_VIEWANGLES;
            continue;
        }
        if (!strcmp(tok, "viewangle_z")) {
            p->s.viewangle_z = parse_int16();
            p->bits |= P_VIEWANGLE2;
            continue;
        }
        if (!strcmp(tok, "kickangles")) {
            parse_int8_v(p->s.kickangles, 3);
            p->bits |= P_KICKANGLES;
            continue;
        }
        if (!strcmp(tok, "weaponindex")) {
            p->s.weaponindex = parse_uint8();
            p->bits |= P_WEAPONINDEX;
            continue;
        }
        if (!strcmp(tok, "weaponframe")) {
            p->s.weaponframe = parse_uint8();
            p->bits |= P_WEAPONFRAME;
            continue;
        }
        if (!strcmp(tok, "gunoffset")) {
            parse_int8_v(p->s.gunoffset, 3);
            p->bits |= P_GUNOFFSET;
            continue;
        }
        if (!strcmp(tok, "gunangles")) {
            parse_int8_v(p->s.gunangles, 3);
            p->bits |= P_GUNANGLES;
            continue;
        }
        if (!strcmp(tok, "blend")) {
            parse_uint8_v(p->s.blend, 4);
            p->bits |= P_BLEND;
            continue;
        }
        if (!strcmp(tok, "fov")) {
            p->s.fov = parse_uint8();
            p->bits |= P_FOV;
            continue;
        }
        if (!strcmp(tok, "rdflags")) {
            p->s.rdflags = parse_uint8();
            p->bits |= P_RDFLAGS;
            continue;
        }
        if (!strcmp(tok, "stats")) {
            p->statbits = parse_stats(p->s.stats);
            p->bits |= P_STATS;
            continue;
        }
        if (!strcmp(tok, "remove")) {
            p->bits |= P_REMOVE;
            continue;
        }
        if (!strcmp(tok, "}")) {
            break;
        }
        unknown();
    }

    if (p->number == CLIENTNUM_NONE) undefined("number");

    return NODE(p);
}

static node_t *parse_player(void)
{
    char *tok;

    tok = parse();
    if (!strcmp(tok, "player")) return _parse_player();
    if (!strcmp(tok, "}")) return NULL;

    unknown();
}

static node_t *_parse_entity(void)
{
    entity_t *e;
    char *tok;

    e = alloc_node(NODE_ENTITY, sizeof(*e));
    e->number = 0;
    e->bits = 0;

    expect("{");
    while (1) {
        tok = parse();
        if (!strcmp(tok, "number")) {
            e->number = parse_uint(1, MAX_EDICTS - 1);
            continue;
        }
        if (!strcmp(tok, "modelindex")) {
            e->s.modelindex = parse_uint8();
            e->bits |= E_MODEL;
            continue;
        }
        if (!strcmp(tok, "modelindex2")) {
            e->s.modelindex2 = parse_uint8();
            e->bits |= E_MODEL2;
            continue;
        }
        if (!strcmp(tok, "modelindex3")) {
            e->s.modelindex3 = parse_uint8();
            e->bits |= E_MODEL3;
            continue;
        }
        if (!strcmp(tok, "modelindex4")) {
            e->s.modelindex4 = parse_uint8();
            e->bits |= E_MODEL4;
            continue;
        }
        if (!strcmp(tok, "frame")) {
            e->s.frame = parse_uint16();
            e->bits |= E_FRAME32;
            continue;
        }
        if (!strcmp(tok, "skin")) {
            e->s.skin = parse_uint32();
            e->bits |= E_SKIN32;
            continue;
        }
        if (!strcmp(tok, "effects")) {
            e->s.effects = parse_uint32();
            e->bits |= E_EFFECTS32;
            continue;
        }
        if (!strcmp(tok, "renderfx")) {
            e->s.renderfx = parse_uint32();
            e->bits |= E_RENDERFX32;
            continue;
        }
        if (!strcmp(tok, "origin_x")) {
            e->s.origin_x = parse_int16();
            e->bits |= E_ORIGIN1;
            continue;
        }
        if (!strcmp(tok, "origin_y")) {
            e->s.origin_y = parse_int16();
            e->bits |= E_ORIGIN2;
            continue;
        }
        if (!strcmp(tok, "origin_z")) {
            e->s.origin_z = parse_int16();
            e->bits |= E_ORIGIN3;
            continue;
        }
        if (!strcmp(tok, "angle_x")) {
            e->s.angle_x = parse_uint8();
            e->bits |= E_ANGLE1;
            continue;
        }
        if (!strcmp(tok, "angle_y")) {
            e->s.angle_y = parse_uint8();
            e->bits |= E_ANGLE2;
            continue;
        }
        if (!strcmp(tok, "angle_z")) {
            e->s.angle_z = parse_uint8();
            e->bits |= E_ANGLE3;
            continue;
        }
        if (!strcmp(tok, "old_origin")) {
            parse_int16_v(e->s.old_origin, 3);
            e->bits |= E_OLDORIGIN;
            continue;
        }
        if (!strcmp(tok, "sound")) {
            e->s.sound = parse_uint8();
            e->bits |= E_SOUND;
            continue;
        }
        if (!strcmp(tok, "event")) {
            e->s.event = parse_uint8();
            e->bits |= E_EVENT;
            continue;
        }
        if (!strcmp(tok, "solid")) {
            e->s.solid = parse_uint16();
            e->bits |= E_SOLID;
            continue;
        }
        if (!strcmp(tok, "remove")) {
            e->bits |= E_REMOVE;
            continue;
        }
        if (!strcmp(tok, "}")) {
            break;
        }
        unknown();
    }

    if (!e->number) {
        fatal("line %d: missing entity number", line.number);
    }

    return NODE(e);
}

static node_t *parse_entity(void)
{
    char *tok;

    tok = parse();
    if (!strcmp(tok, "entity")) return _parse_entity();
    if (!strcmp(tok, "}")) return NULL;

    unknown();
}

static node_t *parse_blob(void)
{
    blob_t *b;
    size_t i, len;
    int c1, c2;

    parse();
    if (!line.len) {
        return NULL;
    }
    if (line.len % 1) {
        fatal("line %d: odd number of characters", line.number);
    }

    len = line.len / 2;
    b = alloc_node(NODE_BLOB, sizeof(*b) + len - 1);
    b->len = len;
    for (i = 0; i < len; i++) {
        c1 = Q_charhex(line.token[i * 2 + 0]);
        c2 = Q_charhex(line.token[i * 2 + 1]);
        if (c1 == -1 || c2 == -1) {
            fatal("line %d: not a hex character", line.number);
        }
        b->data[i] = (c1 << 4) | c2;
    }

    return NODE(b);
}

static node_t *parse_frame(void)
{
    frame_t *f;
    char *tok;

    f = alloc_node(NODE_FRAME, sizeof(*f));
    f->portalbits = NULL;
    f->players = NULL;
    f->entities = NULL;

    expect("{");
    while (1) {
        tok = parse();
        if (!strcmp(tok, "portalbits")) {
            f->portalbits = parse_blob();
        } else if (!strcmp(tok, "players")) {
            expect("{");
            f->players = build_list(parse_player);
        } else if (!strcmp(tok, "entities")) {
            expect("{");
            f->entities = build_list(parse_entity);
        } else if (!strcmp(tok, "}")) {
            break;
        } else {
            unknown();
        }
    }

    return NODE(f);
}

static node_t *parse_configstring(void)
{
    string_t *s;
    char buffer[CS_SIZE(CS_STATUSBAR) + 1];
    char *tok;
    size_t len = SIZE_MAX;
    unsigned index = MAX_CONFIGSTRINGS;

    expect("{");
    while (1) {
        tok = parse();
        if (!strcmp(tok, "index")) {
            index = parse_uint(0, MAX_CONFIGSTRINGS - 1);
        } else if (!strcmp(tok, "string")) {
            len = parse_string(buffer, sizeof(buffer));
        } else if (!strcmp(tok, "}")) {
            break;
        } else {
            unknown();
        }
    }

    if (index == MAX_CONFIGSTRINGS) undefined("index");
    if (len == SIZE_MAX) undefined("string");

    if (len >= CS_SIZE(index)) {
        fatal("line %d: oversize configstring", line.number);
    }

    s = alloc_node(NODE_CONFIGSTRING, sizeof(*s) + len);
    s->index = index;
    memcpy(s->data, buffer, len + 1);
    s->len = len;

    return NODE(s);
}

static node_t *parse_print(void)
{
    string_t *s;
    char buffer[MAX_NET_STRING];
    char *tok;
    size_t len = SIZE_MAX;
    unsigned level = UINT8_MAX + 1;

    expect("{");
    while (1) {
        tok = parse();
        if (!strcmp(tok, "level")) {
            level = parse_uint8();
        } else if (!strcmp(tok, "string")) {
            len = parse_string(buffer, sizeof(buffer));
        } else if (!strcmp(tok, "}")) {
            break;
        } else {
            unknown();
        }
    }

    if (level == UINT8_MAX + 1) undefined("level");
    if (len == SIZE_MAX) undefined("string");

    if (len >= sizeof(buffer)) {
        fatal("line %d: oversize string", line.number);
    }

    s = alloc_node(NODE_PRINT, sizeof(*s) + len);
    s->index = level;
    memcpy(s->data, buffer, len + 1);
    s->len = len;

    return NODE(s);
}


static node_t *parse_svc_string(unsigned type)
{
    string_t *s;

    parse();

    if (line.len >= MAX_NET_STRING) {
        fatal("line %d: oversize string", line.number);
    }

    s = alloc_node(type, sizeof(*s) + line.len);
    memcpy(s->data, line.token, line.len + 1);
    s->len = line.len;

    return NODE(s);
}

static node_t *parse_sound(void)
{
    sound_t *s;
    char *tok;

    s = alloc_node(NODE_SOUND, sizeof(*s));
    s->index = UINT8_MAX + 1;
    s->bits = 0;
    s->flags = 0;
    s->entity = 0;
    s->channel = 0;

    expect("{");
    while (1) {
        tok = parse();
        if (!strcmp(tok, "no_phs_add")) {
            s->flags |= 1;
        } else if (!strcmp(tok, "reliable")) {
            s->flags |= 2;
        } else if (!strcmp(tok, "reserved3")) {
            s->flags |= 4;
        } else if (!strcmp(tok, "index")) {
            s->index = parse_uint8();
        } else if (!strcmp(tok, "volume")) {
            s->bits |= SND_VOLUME;
            s->volume = parse_uint8();
        } else if (!strcmp(tok, "attenuation")) {
            s->bits |= SND_ATTENUATION;
            s->attenuation = parse_uint8();
        } else if (!strcmp(tok, "offset")) {
            s->bits |= SND_OFFSET;
            s->offset = parse_uint8();
        } else if (!strcmp(tok, "entity")) {
            s->bits |= SND_ENT;
            s->entity = parse_uint(0, MAX_EDICTS - 1);
        } else if (!strcmp(tok, "channel")) {
            s->bits |= SND_ENT;
            s->channel = parse_uint(0, 7);
        } else if (!strcmp(tok, "pos")) {
            s->bits |= SND_POS;
            parse_int16_v(s->pos, 3);
        } else if (!strcmp(tok, "}")) {
            break;
        } else {
            unknown();
        }
    }

    if (s->index == UINT8_MAX + 1) undefined("index");

    return NODE(s);
}

static node_t *parse_svc_sound(void)
{
    sound_t *s = (sound_t *)parse_sound();

    if (s->flags) fatal("flags set");

    return NODE(s);
}

static node_t *parse_nop(void)
{
    return alloc_node(NODE_NOP, sizeof(node_t));
}

static node_t *parse_muzzleflash(void)
{
    muzzleflash_t *m;
    char *tok;

    m = alloc_node(NODE_MUZZLEFLASH, sizeof(*m));
    m->type = 0;
    m->entity = MAX_EDICTS;
    m->weapon = UINT8_MAX + 1;

    expect("{");
    while (1) {
        tok = parse();
        if (!strcmp(tok, "type")) {
            m->type = parse_uint(0, 1);
        } else if (!strcmp(tok, "entity")) {
            m->entity = parse_uint(1, MAX_EDICTS - 1);
        } else if (!strcmp(tok, "weapon")) {
            m->weapon = parse_uint8();
        } else if (!strcmp(tok, "}")) {
            break;
        } else {
            unknown();
        }
    }

    if (m->entity == MAX_EDICTS) undefined("entity");
    if (m->weapon == UINT8_MAX + 1) undefined("weapon");

    return NODE(m);
}

static node_t *parse_tent(void)
{
    tent_t *t;
    char *tok;

    t = alloc_node(NODE_TEMP_ENTITY, sizeof(*t));
    memset(t, 0, sizeof(*t));
    t->node.type = NODE_TEMP_ENTITY;

    expect("{");
    while (1) {
        tok = parse();
        if (!strcmp(tok, "type")) {
            t->type = parse_uint(0, TE_NUM_ENTITIES - 1);
        } else if (!strcmp(tok, "pos1")) {
            parse_int16_v(t->pos1, 3);
        } else if (!strcmp(tok, "pos2")) {
            parse_int16_v(t->pos2, 3);
        } else if (!strcmp(tok, "offset")) {
            parse_int16_v(t->offset, 3);
        } else if (!strcmp(tok, "dir")) {
            t->dir = parse_uint8();
        } else if (!strcmp(tok, "count")) {
            t->count = parse_uint8();
        } else if (!strcmp(tok, "color")) {
            t->color = parse_uint8();
        } else if (!strcmp(tok, "entity1")) {
            t->entity1 = parse_uint16();
        } else if (!strcmp(tok, "entity2")) {
            t->entity2 = parse_uint16();
        } else if (!strcmp(tok, "time")) {
            t->time = parse_uint32();
        } else if (!strcmp(tok, "}")) {
            break;
        } else {
            unknown();
        }
    }

    return NODE(t);
}

static node_t *parse_svc(void)
{
    char *tok;

    tok = parse();
    if (!strcmp(tok, "muzzleflash")) return parse_muzzleflash();
    if (!strcmp(tok, "temp_entity")) return parse_tent();
    if (!strcmp(tok, "layout")) return parse_svc_string(NODE_LAYOUT);
    if (!strcmp(tok, "stufftext")) return parse_svc_string(NODE_STUFFTEXT);
    if (!strcmp(tok, "centerprint")) return parse_svc_string(NODE_CENTERPRINT);
    if (!strcmp(tok, "configstring")) return parse_configstring();
    if (!strcmp(tok, "sound")) return parse_svc_sound();
    if (!strcmp(tok, "print")) return parse_print();
    if (!strcmp(tok, "nop")) return parse_nop();
    if (!strcmp(tok, "}")) return NULL;

    unknown();
}

static node_t *parse_multicast(void)
{
    multicast_t *m;
    char *tok;
    node_t **next_p;

    m = alloc_node(NODE_MULTICAST, sizeof(*m));
    m->type = 0;
    m->reliable = false;
    m->data = NULL;

    expect("{");
    while (1) {
        tok = parse();
        if (!strcmp(tok, "leafnum_phs")) {
            m->leafnum = parse_uint16();
            m->type = 1;
        } else if (!strcmp(tok, "leafnum_pvs")) {
            m->leafnum = parse_uint16();
            m->type = 2;
        } else if (!strcmp(tok, "reliable")) {
            m->reliable = true;
        } else if (!strcmp(tok, "data")) {
            expect("{");
            next_p = m->data ? &m->data->next : &m->data;
            *next_p = build_list(parse_svc);
        } else if (!strcmp(tok, "}")) {
            break;
        } else {
            unknown();
        }
    }

    if (!m->data) undefined("data");

    return NODE(m);
}

static node_t *parse_unicast(void)
{
    unicast_t *u;
    char *tok;
    node_t **next_p;

    u = alloc_node(NODE_UNICAST, sizeof(*u));
    u->clientnum = CLIENTNUM_NONE;
    u->reliable = false;
    u->data = NULL;

    expect("{");
    while (1) {
        tok = parse();
        if (!strcmp(tok, "clientnum")) {
            u->clientnum = parse_uint(0, CLIENTNUM_NONE - 1);
        } else if (!strcmp(tok, "reliable")) {
            u->reliable = true;
        } else if (!strcmp(tok, "data")) {
            expect("{");
            next_p = u->data ? &u->data->next : &u->data;
            *next_p = build_list(parse_svc);
        } else if (!strcmp(tok, "}")) {
            break;
        } else {
            unknown();
        }
    }

    if (u->clientnum == CLIENTNUM_NONE) undefined("clientnum");
    if (!u->data) undefined("data");

    return NODE(u);
}

static node_t *parse_basestring(void)
{
    char *tok;

    tok = parse();
    if (!strcmp(tok, "configstring")) return parse_configstring();
    if (!strcmp(tok, "}")) return NULL;

    unknown();
}

static node_t *parse_basestrings(void)
{
    expect("{");
    return build_list(parse_basestring);
}

static node_t *parse_serverdata(void)
{
    game_state_t *g;
    char *tok;

    g = alloc_node(NODE_GAMESTATE, sizeof(*g));
    g->mvdflags = 0;
    g->majorversion = 0;
    g->minorversion = 0;
    g->servercount = 0;
    g->gamedir[0] = 0;
    g->clientnum = CLIENTNUM_NONE;
    g->configstrings = NULL;
    g->baseframe = NULL;
    expect("{");
    while (1) {
        tok = parse();
        if (!strcmp(tok, "no_msgs")) {
            g->mvdflags |= 1;
        } else if (!strcmp(tok, "reserved2")) {
            g->mvdflags |= 2;
        } else if (!strcmp(tok, "reserved3")) {
            g->mvdflags |= 4;
        } else if (!strcmp(tok, "majorversion")) {
            g->majorversion = parse_uint32();
            if (g->majorversion != PROTOCOL_VERSION_MVD) {
                fatal("unknown major protocol version");
            }
        } else if (!strcmp(tok, "minorversion")) {
            g->minorversion = parse_uint16();
            if (!MVD_SUPPORTED(g->minorversion)) {
                fatal("unsupported minor protocol version");
            }
        } else if (!strcmp(tok, "servercount")) {
            g->servercount = parse_uint32();
        } else if (!strcmp(tok, "gamedir")) {
            parse_string(g->gamedir, MAX_QPATH);
        } else if (!strcmp(tok, "clientnum")) {
            g->clientnum = parse_int(-1, CLIENTNUM_NONE - 1);
        } else if (!strcmp(tok, "basestrings")) {
            g->configstrings = parse_basestrings();
        } else if (!strcmp(tok, "baseframe")) {
            g->baseframe = parse_frame();
        } else if (!strcmp(tok, "}")) {
            break;
        } else {
            unknown();
        }
    }

    if (!g->majorversion) undefined("majorversion");
    if (!g->minorversion) undefined("minorversion");
    if (g->clientnum == CLIENTNUM_NONE) undefined("clientnum");
    if (!g->configstrings) undefined("basestrings");
    if (!g->baseframe) undefined("baseframe");

    return NODE(g);
}

static node_t *parse_mvd_sound(void)
{
    sound_t *s = (sound_t *)parse_sound();

    if (!(s->bits & SND_ENT)) undefined("channel");
    if (s->bits & SND_POS) fatal("pos set");
    //s->bits &= ~SND_ENT;

    return NODE(s);
}

static node_t *parse_message(void)
{
    char *tok;

    tok = parse();
    if (!strcmp(tok, "serverdata")) return parse_serverdata();
    if (!strcmp(tok, "multicast")) return parse_multicast();
    if (!strcmp(tok, "unicast")) return parse_unicast();
    if (!strcmp(tok, "configstring")) return parse_configstring();
    if (!strcmp(tok, "frame")) return parse_frame();
    if (!strcmp(tok, "sound")) return parse_mvd_sound();
    if (!strcmp(tok, "print")) return parse_print();
    if (!strcmp(tok, "nop")) return parse_nop();
    if (!strcmp(tok, "}")) return NULL;

    unknown();
}

node_t *read_txt(FILE *fp)
{
    char *tok;

    ifp = fp;
    tok = parse();
    if (!strcmp(tok, "EOF")) {
        return NULL;
    }
    if (strcmp(tok, "block")) {
        fatal("line %d: expected block, but got %s",
              line.number, tok);
    }
    expect("{");
    return build_list(parse_message);
}

//
// writing
//

static struct {
    int indent;
    const char *names[16];
} block;

static char *make_indent(void)
{
    static char indent[16];

    memset(indent, ' ', block.indent);
    indent[block.indent] = 0;

    return indent;
}

static void begin_block(const char *name)
{
    fprintf(ofp, "%s%s {\n", make_indent(), name);
    block.names[block.indent++] = name;
}

static void end_block(void)
{
    block.indent--;
    //fprintf(ofp, "%s} // %s\n", make_indent(), block.names[block.indent]);
    fprintf(ofp, "%s}\n", make_indent());
}

static void write_tok(const char *tok)
{
    fprintf(ofp, "%s%s\n", make_indent(), tok);
}

static void write_int(const char *name, int v)
{
    fprintf(ofp, "%s%s %d\n", make_indent(), name, v);
}

static void write_hex(const char *name, int v)
{
    fprintf(ofp, "%s%s %#x\n", make_indent(), name, v);
}

static void write_int_v(const char *name, int *v, size_t n)
{
    size_t i;

    fprintf(ofp, "%s%s ", make_indent(), name);
    for (i = 0; i < n; i++) {
        fprintf(ofp, "%d ", v[i]);
    }
    fprintf(ofp, "\n");
}

static void write_uint_v(const char *name, unsigned *v, size_t n)
{
    size_t i;

    fprintf(ofp, "%s%s ", make_indent(), name);
    for (i = 0; i < n; i++) {
        fprintf(ofp, "%u ", v[i]);
    }
    fprintf(ofp, "\n");
}

static void write_string(const char *name, const char *s)
{
    static const char hex[] = "0123456789ABCDEF";
    char buffer[MAX_NET_STRING * 4], *p;
    int c;

    p = buffer;
    while (*s) {
        c = (uint8_t)*s++;

        switch (c) {
        case '\"':
            *p++ = '\\';
            *p++ = '\"';
            break;
        case '\\':
            *p++ = '\\';
            *p++ = '\\';
            break;
        case '\t':
            *p++ = '\\';
            *p++ = 't';
            break;
        case '\r':
            *p++ = '\\';
            *p++ = 'r';
            break;
        case '\n':
            *p++ = '\\';
            *p++ = 'n';
            break;
        default:
            if (c >= 32 && c < 127) {
                *p++ = c;
            } else {
                *p++ = '\\';
                *p++ = 'x';
                *p++ = hex[c >> 4];
                *p++ = hex[c & 15];
            }
            break;
        }
    }
    *p = 0;

    fprintf(ofp, "%s%s \"%s\"\n", make_indent(), name, buffer);
}

static void write_blob(const char *name, blob_t *b)
{
    size_t i;

    if (!b) {
        return;
    }

    fprintf(ofp, "%s%s ", make_indent(), name);
    for (i = 0; i < b->len; i++) {
        fprintf(ofp, "%02x", i[b->data]);
    }
    fprintf(ofp, "\n");
}

static void write_stats(player_t *p)
{
    int i;

    begin_block("stats");
    for (i = 0; i < MAX_STATS; i++) {
        if (p->statbits & (1U << i)) {
            begin_block("stat");
            write_int("index", i);
            write_int("value", p->s.stats[i]);
            end_block();
        }
    }
    end_block();
}

static void write_player(player_t *p)
{
    begin_block("player");
    write_int("number", p->number);
    if (p->bits & P_TYPE) write_int("pm_type", p->s.pm_type);
    if (p->bits & P_ORIGIN) write_int_v("origin_xy", p->s.origin_xy, 2);
    if (p->bits & P_ORIGIN2) write_int("origin_z", p->s.origin_z);
    if (p->bits & P_VIEWOFFSET) write_int_v("viewoffset", p->s.viewoffset, 3);
    if (p->bits & P_VIEWANGLES) write_int_v("viewangles_xy", p->s.viewangles_xy, 2);
    if (p->bits & P_VIEWANGLE2) write_int("viewangle_z", p->s.viewangle_z);
    if (p->bits & P_KICKANGLES) write_int_v("kickangles", p->s.kickangles, 3);
    if (p->bits & P_WEAPONINDEX) write_int("weaponindex", p->s.weaponindex);
    if (p->bits & P_WEAPONFRAME) write_int("weaponframe", p->s.weaponframe);
    if (p->bits & P_GUNOFFSET) write_int_v("gunoffset", p->s.gunoffset, 3);
    if (p->bits & P_GUNANGLES) write_int_v("gunangles", p->s.gunangles, 3);
    if (p->bits & P_BLEND) write_uint_v("blend", p->s.blend, 4);
    if (p->bits & P_FOV) write_int("fov", p->s.fov);
    if (p->bits & P_RDFLAGS) write_int("rdflags", p->s.rdflags);
    if (p->bits & P_STATS) write_stats(p);
    if (p->bits & P_REMOVE) write_tok("remove");
    end_block();
}

static void write_entity(entity_t *e)
{
    begin_block("entity");
    write_int("number", e->number);
    if (e->bits & E_MODEL) write_int("modelindex", e->s.modelindex);
    if (e->bits & E_MODEL2) write_int("modelindex2", e->s.modelindex2);
    if (e->bits & E_MODEL3) write_int("modelindex3", e->s.modelindex3);
    if (e->bits & E_MODEL4) write_int("modelindex4", e->s.modelindex4);
    if (e->bits & E_FRAME32) write_int("frame", e->s.frame);
    if (e->bits & E_SKIN32) write_int("skin", e->s.skin);
    if (e->bits & E_EFFECTS32) write_hex("effects", e->s.effects);
    if (e->bits & E_RENDERFX32) write_hex("renderfx", e->s.renderfx);
    if (e->bits & E_ORIGIN1) write_int("origin_x", e->s.origin_x);
    if (e->bits & E_ORIGIN2) write_int("origin_y", e->s.origin_y);
    if (e->bits & E_ORIGIN3) write_int("origin_z", e->s.origin_z);
    if (e->bits & E_ANGLE1) write_int("angle_x", e->s.angle_x);
    if (e->bits & E_ANGLE2) write_int("angle_y", e->s.angle_y);
    if (e->bits & E_ANGLE3) write_int("angle_z", e->s.angle_z);
    if (e->bits & E_OLDORIGIN) write_int_v("old_origin", e->s.old_origin, 3);
    if (e->bits & E_SOUND) write_int("sound", e->s.sound);
    if (e->bits & E_EVENT) write_int("event", e->s.event);
    if (e->bits & E_SOLID) write_int("solid", e->s.solid);
    if (e->bits & E_REMOVE) write_tok("remove");
    end_block();
}

static void write_frame(const char *name, frame_t *f)
{
    begin_block(name);
    write_blob("portalbits", (blob_t *)f->portalbits);
    if (f->players) {
        begin_block("players");
        iter_list(f->players, write_player);
        end_block();
    }
    if (f->entities) {
        begin_block("entities");
        iter_list(f->entities, write_entity);
        end_block();
    }
    end_block();
}

static void write_configstring(string_t *s)
{
    begin_block("configstring");
    write_int("index", s->index);
    write_string("string", s->data);
    end_block();
}

static void write_game_state(game_state_t *g)
{
    begin_block("serverdata");
    write_int("majorversion", g->majorversion);
    write_int("minorversion", g->minorversion);
    write_hex("servercount", g->servercount);
    write_string("gamedir", g->gamedir);
    write_int("clientnum", g->clientnum);
    if (g->mvdflags & 1) write_tok("no_msgs");
    if (g->mvdflags & 2) write_tok("reserved2");
    if (g->mvdflags & 4) write_tok("reserved3");
    begin_block("basestrings");
    iter_list(g->configstrings, write_configstring);
    end_block();
    write_frame("baseframe", (frame_t *)g->baseframe);
    end_block();
}

static void write_print(string_t *s)
{
    begin_block("print");
    write_int("level", s->index);
    write_string("string", s->data);
    end_block();
}

static void write_muzzleflash(muzzleflash_t *m)
{
    begin_block("muzzleflash");
    write_int("type", m->type);
    write_int("entity", m->entity);
    write_int("weapon", m->weapon);
    end_block();
}

static void write_svc_sound(sound_t *s)
{
    begin_block("sound");
    write_int("index", s->index);
    if (s->bits & SND_VOLUME) write_int("volume", s->volume);
    if (s->bits & SND_ATTENUATION) write_int("attenuation", s->attenuation);
    if (s->bits & SND_OFFSET) write_int("offset", s->offset);
    if (s->bits & SND_ENT) {
        write_int("entity", s->entity);
        write_int("channel", s->channel);
    }
    if (s->bits & SND_POS) write_int_v("pos", s->pos, 3);
    end_block();
}

static void write_temp_entity(tent_t *t)
{
    begin_block("temp_entity");
    write_int("type", t->type);

    switch (t->type) {
    case TE_BLOOD:
    case TE_GUNSHOT:
    case TE_SPARKS:
    case TE_BULLET_SPARKS:
    case TE_SCREEN_SPARKS:
    case TE_SHIELD_SPARKS:
    case TE_SHOTGUN:
    case TE_BLASTER:
    case TE_GREENBLOOD:
    case TE_BLASTER2:
    case TE_FLECHETTE:
    case TE_HEATBEAM_SPARKS:
    case TE_HEATBEAM_STEAM:
    case TE_MOREBLOOD:
    case TE_ELECTRIC_SPARKS:
        write_int_v("pos1", t->pos1, 3);
        write_int("dir", t->dir);
        break;

    case TE_SPLASH:
    case TE_LASER_SPARKS:
    case TE_WELDING_SPARKS:
    case TE_TUNNEL_SPARKS:
        write_int("count", t->count);
        write_int_v("pos1", t->pos1, 3);
        write_int("dir", t->dir);
        write_int("color", t->color);
        break;

    case TE_BLUEHYPERBLASTER:
    case TE_RAILTRAIL:
    case TE_BUBBLETRAIL:
    case TE_DEBUGTRAIL:
    case TE_BUBBLETRAIL2:
    case TE_BFG_LASER:
        write_int_v("pos1", t->pos1, 3);
        write_int_v("pos2", t->pos2, 3);
        break;

    case TE_GRENADE_EXPLOSION:
    case TE_GRENADE_EXPLOSION_WATER:
    case TE_EXPLOSION2:
    case TE_PLASMA_EXPLOSION:
    case TE_ROCKET_EXPLOSION:
    case TE_ROCKET_EXPLOSION_WATER:
    case TE_EXPLOSION1:
    case TE_EXPLOSION1_NP:
    case TE_EXPLOSION1_BIG:
    case TE_BFG_EXPLOSION:
    case TE_BFG_BIGEXPLOSION:
    case TE_BOSSTPORT:
    case TE_PLAIN_EXPLOSION:
    case TE_CHAINFIST_SMOKE:
    case TE_TRACKER_EXPLOSION:
    case TE_TELEPORT_EFFECT:
    case TE_DBALL_GOAL:
    case TE_WIDOWSPLASH:
    case TE_NUKEBLAST:
        write_int_v("pos1", t->pos1, 3);
        break;

    case TE_PARASITE_ATTACK:
    case TE_MEDIC_CABLE_ATTACK:
    case TE_HEATBEAM:
    case TE_MONSTER_HEATBEAM:
        write_int("entity1", t->entity1);
        write_int_v("pos1", t->pos1, 3);
        write_int_v("pos2", t->pos2, 3);
        break;

    case TE_GRAPPLE_CABLE:
        write_int("entity1", t->entity1);
        write_int_v("pos1", t->pos1, 3);
        write_int_v("pos2", t->pos2, 3);
        write_int_v("offset", t->offset, 3);
        break;

    case TE_LIGHTNING:
        write_int("entity1", t->entity1);
        write_int("entity2", t->entity2);
        write_int_v("pos1", t->pos1, 3);
        write_int_v("pos2", t->pos2, 3);
        break;

    case TE_FLASHLIGHT:
        write_int_v("pos1", t->pos1, 3);
        write_int("entity1", t->entity1);
        break;

    case TE_FORCEWALL:
        write_int_v("pos1", t->pos1, 3);
        write_int_v("pos2", t->pos2, 3);
        write_int("color", t->color);
        break;

    case TE_STEAM:
        write_int("entity1", t->entity1);
        write_int("count", t->count);
        write_int_v("pos1", t->pos1, 3);
        write_int("dir", t->dir);
        write_int("color", t->color);
        write_int("entity2", t->entity2);
        if (t->entity1 != 0xffff) {
            write_int("time", t->time);
        }
        break;

    case TE_WIDOWBEAMOUT:
        write_int("entity1", t->entity1);
        write_int_v("pos1", t->pos1, 3);
        break;

    default:
        fatal("unknown temp entity type");
    }
    end_block();
}

static void write_svc(void *n)
{
    switch (((node_t *)n)->type) {
    case NODE_MUZZLEFLASH:
        write_muzzleflash(n);
        break;
    case NODE_TEMP_ENTITY:
        write_temp_entity(n);
        break;
    case NODE_LAYOUT:
        write_string("layout", ((string_t *)n)->data);
        break;
    case NODE_STUFFTEXT:
        write_string("stufftext", ((string_t *)n)->data);
        break;
    case NODE_CENTERPRINT:
        write_string("centerprint", ((string_t *)n)->data);
        break;
    case NODE_CONFIGSTRING:
        write_configstring(n);
        break;
    case NODE_SOUND:
        write_svc_sound(n);
        break;
    case NODE_PRINT:
        write_print(n);
        break;
    case NODE_NOP:
        write_tok("nop");
        break;
    default:
        fatal("bad node type");
    }
}

static void write_data(node_t *n)
{
    begin_block("data");
    iter_list(n, write_svc);
    end_block();
}

static void write_multicast(multicast_t *m)
{
    begin_block("multicast");
    if (m->reliable) write_tok("reliable");
    switch (m->type) {
    case 1:
        write_int("leafnum_phs", m->leafnum);
        break;
    case 2:
        write_int("leafnum_pvs", m->leafnum);
        break;
    }
    write_data(m->data);
    end_block();
}

static void write_unicast(unicast_t *u)
{
    begin_block("unicast");
    if (u->reliable) write_tok("reliable");
    write_int("clientnum", u->clientnum);
    write_data(u->data);
    end_block();
}

static void write_mvd_sound(sound_t *s)
{
    begin_block("sound");
    write_int("index", s->index);
    if (s->flags & 1) write_tok("no_phs_add");
    if (s->flags & 2) write_tok("reliable");
    if (s->flags & 4) write_tok("reserved3");
    if (s->bits & SND_VOLUME) write_int("volume", s->volume);
    if (s->bits & SND_ATTENUATION) write_int("attenuation", s->attenuation);
    if (s->bits & SND_OFFSET) write_int("offset", s->offset);
    write_int("entity", s->entity);
    write_int("channel", s->channel);
    end_block();
}

static void write_node(void *n)
{
    switch (((node_t *)n)->type) {
    case NODE_GAMESTATE:
        write_game_state(n);
        break;
    case NODE_MULTICAST:
        write_multicast(n);
        break;
    case NODE_UNICAST:
        write_unicast(n);
        break;
    case NODE_CONFIGSTRING:
        write_configstring(n);
        break;
    case NODE_FRAME:
        write_frame("frame", n);
        break;
    case NODE_SOUND:
        write_mvd_sound(n);
        break;
    case NODE_PRINT:
        write_print(n);
        break;
    case NODE_NOP:
        write_tok("nop");
        break;
    default:
        fatal("bad node type");
    }
}

void write_txt(FILE *fp, node_t *nodes, unsigned blocknum)
{
    ofp = fp;

    if (!nodes) {
        fprintf(ofp, "\nEOF\n");
        return;
    }

    block.indent = 1;
    fprintf(ofp, "\nblock { // %u\n", blocknum);
    iter_list(nodes, write_node);
    end_block();
}

