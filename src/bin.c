#include "main.h"
#include "swap.h"
#include "proto.h"
#include "node.h"
#include "bin.h"

static struct {
    uint8_t data[MAX_MSGLEN];
    size_t head, tail;
} msg;

static demo_t *current;

//
// reading
//

static void *read_data(size_t len)
{
    void *p;

    if (len > msg.tail - msg.head) {
        fatal("read past end of message");
    }

    p = msg.data + msg.head;
    msg.head += len;
    return p;
}

static unsigned read_uint8(void)
{
    uint8_t *p = read_data(1);
    return *p;
}

static int read_int16(void)
{
    uint8_t *p = read_data(2);
    return (int16_t)(p[0] | (p[1] << 8));
}

static unsigned read_uint16(void)
{
    uint8_t *p = read_data(2);
    return (uint16_t)(p[0] | (p[1] << 8));
}

static unsigned read_uint32(void)
{
    uint8_t *p = read_data(4);
    return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int *read_int8_v(int *v, size_t n)
{
    uint8_t *p = read_data(n);
    size_t i;

    for (i = 0; i < n; i++) {
        v[i] = (int8_t)p[i];
    }

    return v;
}

static unsigned *read_uint8_v(unsigned *v, size_t n)
{
    uint8_t *p = read_data(n);
    size_t i;

    for (i = 0; i < n; i++) {
        v[i] = p[i];
    }

    return v;
}

static int *read_int16_v(int *v, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++) {
        v[i] = read_int16();
    }

    return v;
}

static size_t read_string(char *dest, size_t size)
{
    size_t len = 0;
    int c;

    while (1) {
        c = read_uint8();
        if (!c) {
            break;
        }
        if (len + 1 < size) {
            *dest++ = c;
        }
        len++;
    }

    if (size) {
        *dest = 0;
    }
    return len;
}

static node_t *parse_entity(void)
{
    entity_t *e;
    unsigned bits;
    unsigned number;

    bits = read_uint8();
    if (bits & E_MOREBITS1) bits |= read_uint8() << 8;
    if (bits & E_MOREBITS2) bits |= read_uint8() << 16;
    if (bits & E_MOREBITS3) bits |= read_uint8() << 24;

    if (bits & E_NUMBER16) {
        number = read_uint16();
    } else {
        number = read_uint8();
    }

    if (!number) {
        return NULL;
    }

    if (number >= MAX_EDICTS) {
        fatal("bad entity number");
    }

    e = alloc_node(NODE_ENTITY, sizeof(*e));
    e->number = number;
    e->bits = bits;

    if (bits & E_MODEL) e->s.modelindex = read_uint8();
    if (bits & E_MODEL2) e->s.modelindex2 = read_uint8();
    if (bits & E_MODEL3) e->s.modelindex3 = read_uint8();
    if (bits & E_MODEL4) e->s.modelindex4 = read_uint8();
    switch (bits & E_FRAME32) {
    case E_FRAME32:
        fatal("E_FRAME32 is not supported");
    case E_FRAME16:
        e->s.frame = read_uint16();
        break;
    case E_FRAME8:
        e->s.frame = read_uint8();
        break;
    }
    switch (bits & E_SKIN32) {
    case E_SKIN32:
        e->s.skin = read_uint32();
        break;
    case E_SKIN16:
        e->s.skin = read_uint16();
        break;
    case E_SKIN8:
        e->s.skin = read_uint8();
        break;
    }
    switch (bits & E_EFFECTS32) {
    case E_EFFECTS32:
        e->s.effects = read_uint32();
        break;
    case E_EFFECTS16:
        e->s.effects = read_uint16();
        break;
    case E_EFFECTS8:
        e->s.effects = read_uint8();
        break;
    }
    switch (bits & E_RENDERFX32) {
    case E_RENDERFX32:
        e->s.renderfx = read_uint32();
        break;
    case E_RENDERFX16:
        e->s.renderfx = read_uint16();
        break;
    case E_RENDERFX8:
        e->s.renderfx = read_uint8();
        break;
    }
    if (bits & E_ORIGIN1) e->s.origin_x = read_int16();
    if (bits & E_ORIGIN2) e->s.origin_y = read_int16();
    if (bits & E_ORIGIN3) e->s.origin_z = read_int16();
    if (bits & E_ANGLE1) e->s.angle_x = read_uint8();
    if (bits & E_ANGLE2) e->s.angle_y = read_uint8();
    if (bits & E_ANGLE3) e->s.angle_z = read_uint8();
    if (bits & E_OLDORIGIN) read_int16_v(e->s.old_origin, 3);
    if (bits & E_SOUND) e->s.sound = read_uint8();
    if (bits & E_EVENT) e->s.event = read_uint8();
    if (bits & E_SOLID) e->s.solid = read_int16();

    return NODE(e);
}

static node_t *parse_player(void)
{
    player_t *p;
    unsigned bits;
    unsigned number;
    int i;

    number = read_uint8();
    if (number == CLIENTNUM_NONE) {
        return NULL;
    }
    if (number >= MAX_CLIENTS) {
        fatal("bad client number");
    }

    bits = read_int16();

    p = alloc_node(NODE_PLAYER, sizeof(*p));
    p->number = number;
    p->bits = bits;

    if (bits & P_TYPE) p->s.pm_type = read_uint8();
    if (bits & P_ORIGIN) read_int16_v(p->s.origin, 2);
    if (bits & P_ORIGIN2) p->s.origin[2] = read_int16();
    if (bits & P_VIEWOFFSET) read_int8_v(p->s.viewoffset, 3);
    if (bits & P_VIEWANGLES) read_int16_v(p->s.viewangles, 2);
    if (bits & P_VIEWANGLE2) p->s.viewangles[2] = read_int16();
    if (bits & P_KICKANGLES) read_int8_v(p->s.kickangles, 3);
    if (bits & P_WEAPONINDEX) p->s.weaponindex = read_uint8();
    if (bits & P_WEAPONFRAME) p->s.weaponframe = read_uint8();
    if (bits & P_GUNOFFSET) read_int8_v(p->s.gunoffset, 3);
    if (bits & P_GUNANGLES) read_int8_v(p->s.gunangles, 3);
    if (bits & P_BLEND) read_uint8_v(p->s.blend, 4);
    if (bits & P_FOV) p->s.fov = read_uint8();
    if (bits & P_RDFLAGS) p->s.rdflags = read_uint8();
    if (bits & P_STATS) {
        p->statbits = read_uint32();
        for (i = 0; i < MAX_STATS; i++) {
            if (p->statbits & (1U << i)) {
                p->s.stats[i] = read_int16();
            }
        }
    }

    return NODE(p);
}

static node_t *parse_blob(void)
{
    blob_t *b;
    size_t len;

    len = read_uint8();
    if (!len) {
        return NULL;
    }

    b = alloc_node(NODE_BLOB, sizeof(*b) + len - 1);
    b->len = len;
    memcpy(b->data, read_data(len), len);

    return NODE(b);
}

static node_t *parse_frame(void)
{
    frame_t *f = alloc_node(NODE_FRAME, sizeof(*f));

    f->portalbits = parse_blob();
    f->players = build_list(parse_player);
    f->entities = build_list(parse_entity);

    return NODE(f);
}

static node_t *_parse_configstring(void)
{
    char buffer[CS_SIZE(CS_STATUSBAR) + 1];
    unsigned index;
    string_t *s;
    size_t len;

    index = read_uint16();
    if (index == MAX_CONFIGSTRINGS) {
        return NULL;
    }
    if (index > MAX_CONFIGSTRINGS) {
        fatal("bad configstring index");
    }

    len = read_string(buffer, sizeof(buffer));
    if (len >= CS_SIZE(index)) {
        fatal("oversize confistring");
    }

    s = alloc_node(NODE_CONFIGSTRING, sizeof(*s) + len);
    s->index = index;
    memcpy(s->data, buffer, len + 1);
    s->len = len;

    return NODE(s);
}

static node_t *parse_serverdata(unsigned bits)
{
    game_state_t *g;

    g = alloc_node(NODE_GAMESTATE, sizeof(*g));
    g->majorversion = read_uint32();
    if (g->majorversion != PROTOCOL_VERSION_MVD) {
        fatal("unknown major protocol version");
    }

    g->minorversion = read_uint16();
    if (!MVD_SUPPORTED(g->minorversion)) {
        fatal("unsupported minor protocol version");
    }

    g->servercount = read_uint32();
    read_string(g->gamedir, sizeof(g->gamedir));
    g->clientnum = read_int16();
    g->mvdflags = bits;
    g->configstrings = build_list(_parse_configstring);
    g->baseframe = parse_frame();

    return NODE(g);
}

static string_t *_parse_string(unsigned type)
{
    string_t *s;
    char buffer[MAX_NET_STRING];
    size_t len;

    len = read_string(buffer, sizeof(buffer));
    if (len >= sizeof(buffer)) {
        fatal("oversize string");
    }

    s = alloc_node(type, sizeof(*s) + len);
    memcpy(s->data, buffer, len + 1);
    s->len = len;

    return s;
}

#define parse_string(x) NODE(_parse_string(x))

static node_t *parse_print(void)
{
    string_t *s;
    unsigned level;

    level = read_uint8();
    s = _parse_string(NODE_PRINT);
    s->index = level;

    return NODE(s);
}

static node_t *parse_muzzleflash(unsigned cmd)
{
    muzzleflash_t *m = alloc_node(NODE_MUZZLEFLASH, sizeof(*m));

    m->type = cmd - svc_muzzleflash;
    m->entity = read_uint16();
    m->weapon = read_uint8();

    return NODE(m);
}

static node_t *parse_svc_sound(void)
{
    sound_t *s = alloc_node(NODE_SOUND, sizeof(*s));

    s->bits = read_uint8();
    s->index = read_uint8();
    if (s->bits & SND_VOLUME) s->volume = read_uint8();
    if (s->bits & SND_ATTENUATION) s->attenuation = read_uint8();
    if (s->bits & SND_OFFSET) s->offset = read_uint8();
    if (s->bits & SND_ENT) {
        unsigned channel = read_uint16();
        s->entity = channel >> 3;
        s->channel = channel & 7;
    }
    if (s->bits & SND_POS) read_int16_v(s->pos, 3);

    return NODE(s);
}

static node_t *parse_configstring(void)
{
    node_t *s = _parse_configstring();

    if (!s) {
        fatal("bad configstring index");
    }
    return s;
}

static node_t *parse_temp_entity(void)
{
    tent_t *t = alloc_node(NODE_TEMP_ENTITY, sizeof(*t));

    t->type = read_uint8();
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
        read_int16_v(t->pos1, 3);
        t->dir = read_uint8();
        break;

    case TE_SPLASH:
    case TE_LASER_SPARKS:
    case TE_WELDING_SPARKS:
    case TE_TUNNEL_SPARKS:
        t->count = read_uint8();
        read_int16_v(t->pos1, 3);
        t->dir = read_uint8();
        t->color = read_uint8();
        break;

    case TE_BLUEHYPERBLASTER:
    case TE_RAILTRAIL:
    case TE_BUBBLETRAIL:
    case TE_DEBUGTRAIL:
    case TE_BUBBLETRAIL2:
    case TE_BFG_LASER:
        read_int16_v(t->pos1, 3);
        read_int16_v(t->pos2, 3);
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
        read_int16_v(t->pos1, 3);
        break;

    case TE_PARASITE_ATTACK:
    case TE_MEDIC_CABLE_ATTACK:
    case TE_HEATBEAM:
    case TE_MONSTER_HEATBEAM:
        t->entity1 = read_uint16();
        read_int16_v(t->pos1, 3);
        read_int16_v(t->pos2, 3);
        break;

    case TE_GRAPPLE_CABLE:
        t->entity1 = read_uint16();
        read_int16_v(t->pos1, 3);
        read_int16_v(t->pos2, 3);
        read_int16_v(t->offset, 3);
        break;

    case TE_LIGHTNING:
        t->entity1 = read_uint16();
        t->entity2 = read_uint16();
        read_int16_v(t->pos1, 3);
        read_int16_v(t->pos2, 3);
        break;

    case TE_FLASHLIGHT:
        read_int16_v(t->pos1, 3);
        t->entity1 = read_uint16();
        break;

    case TE_FORCEWALL:
        read_int16_v(t->pos1, 3);
        read_int16_v(t->pos2, 3);
        t->color = read_uint8();
        break;

    case TE_STEAM:
        t->entity1 = read_uint16();
        t->count = read_uint8();
        read_int16_v(t->pos1, 3);
        t->dir = read_uint8();
        t->color = read_uint8();
        t->entity2 = read_uint16();
        if (t->entity1 != 0xffff) {
            t->time = read_uint32();
        }
        break;

    case TE_WIDOWBEAMOUT:
        t->entity1 = read_uint16();
        read_int16_v(t->pos1, 3);
        break;

    default:
        fatal("unknown temp entity type");
    }

    return NODE(t);
}

static node_t *parse_inventory(void)
{
    inventory_t *n = alloc_node(NODE_INVENTORY, sizeof(*n));

    read_int16_v(n->inventory, MAX_ITEMS);

    return NODE(n);
}

static node_t *_parse_svc(unsigned cmd)
{
    node_t *n;

    switch (cmd) {
    case svc_muzzleflash:
    case svc_muzzleflash2:
        n = parse_muzzleflash(cmd);
        break;
    case svc_temp_entity:
        n = parse_temp_entity();
        break;
    case svc_layout:
        n = parse_string(NODE_LAYOUT);
        break;
    case svc_stufftext:
        n = parse_string(NODE_STUFFTEXT);
        break;
    case svc_centerprint:
        n = parse_string(NODE_CENTERPRINT);
        break;
    case svc_inventory:
        n = parse_inventory();
        break;
    case svc_sound:
        n = parse_svc_sound();
        break;
    case svc_print:
        n = parse_print();
        break;
    case svc_configstring:
        n = parse_configstring();
        break;
    case svc_nop:
        n = alloc_node(NODE_NOP, sizeof(*n));
        break;
    default:
        fatal("unknown SVC command byte");
    }

    return n;
}

static node_t *parse_svc(size_t tail)
{
    node_t *ret, *n, **next_p;

    if (tail > msg.tail) {
        fatal("read past end of message");
    }

    next_p = &ret;
    while (msg.head < tail) {
        n = _parse_svc(read_uint8());
        *next_p = n;
        next_p = &n->next;
    }
    *next_p = NULL;

    return ret;
}

static node_t *parse_multicast(unsigned cmd, unsigned bits)
{
    multicast_t *m;
    size_t len;

    m = alloc_node(NODE_MULTICAST, sizeof(*m));
    m->type = (cmd - mvd_multicast_all) % 3;
    m->reliable = cmd < mvd_multicast_all_r ? false : true;

    len = read_uint8();
    len |= bits << 8;

    if (m->type) {
        m->leafnum = read_uint16();
    }
    m->data = parse_svc(msg.head + len);

    return NODE(m);
}

static node_t *parse_unicast(unsigned cmd, unsigned bits)
{
    unicast_t *u;
    size_t len;

    u = alloc_node(NODE_UNICAST, sizeof(*u));
    u->reliable = cmd - mvd_unicast;

    len = read_uint8();
    len |= bits << 8;

    u->clientnum = read_uint8();
    u->data = parse_svc(msg.head + len);

    return NODE(u);
}

static node_t *parse_mvd_sound(unsigned bits)
{
    sound_t *s = alloc_node(NODE_SOUND, sizeof(*s));
    unsigned channel;

    s->flags = bits;
    s->bits = read_uint8();
    s->index = read_uint8();
    if (s->bits & SND_VOLUME) s->volume = read_uint8();
    if (s->bits & SND_ATTENUATION) s->attenuation = read_uint8();
    if (s->bits & SND_OFFSET) s->offset = read_uint8();
    channel = read_uint16();
    s->entity = channel >> 3;
    s->channel = channel & 7;

    return NODE(s);
}

static node_t *parse_mvd_message(void)
{
    unsigned cmd, bits;
    node_t *n;

    if (msg.head >= msg.tail)
        return NULL;

    cmd = read_uint8();
    bits = cmd >> SVCMD_BITS;
    cmd &= SVCMD_MASK;

    switch (cmd) {
    case mvd_serverdata:
        n = parse_serverdata(bits);
        break;
    case mvd_multicast_all:
    case mvd_multicast_phs:
    case mvd_multicast_pvs:
    case mvd_multicast_all_r:
    case mvd_multicast_phs_r:
    case mvd_multicast_pvs_r:
        n = parse_multicast(cmd, bits);
        break;
    case mvd_unicast:
    case mvd_unicast_r:
        n = parse_unicast(cmd, bits);
        break;
    case mvd_configstring:
        n = parse_configstring();
        break;
    case mvd_frame:
        n = parse_frame();
        break;
    case mvd_sound:
        n = parse_mvd_sound(bits);
        break;
    case mvd_print:
        n = parse_print();
        break;
    case mvd_nop:
        n = alloc_node(NODE_NOP, sizeof(*n));
        break;
    default:
        fatal("unknown MVD command byte");
    }

    return n;
}

static node_t *parse_svc_serverdata(void)
{
    serverdata_t *s;

    s = alloc_node(NODE_SERVERDATA, sizeof(*s));
    s->majorversion = read_uint32();
    if (s->majorversion < PROTOCOL_VERSION_OLD || s->majorversion > PROTOCOL_VERSION_DEFAULT) {
        fatal("unknown major protocol version");
    }
    s->servercount = read_uint32();
    s->attractloop = read_uint8();
    read_string(s->gamedir, sizeof(s->gamedir));
    s->clientnum = read_int16();
    read_string(s->levelname, sizeof(s->levelname));
    current->protocol = s->majorversion;

    return NODE(s);
}

static node_t *parse_svc_player(void)
{
    player_t *p;
    unsigned bits;
    int i;

    bits = read_int16();

    p = alloc_node(NODE_PLAYER, sizeof(*p));
    p->number = CLIENTNUM_NONE;
    p->bits = bits;

    if (bits & PS_TYPE) {
        p->s.pm_type = read_uint8();
    }
    if (bits & PS_ORIGIN) {
        read_int16_v(p->s.origin, 3);
    }
    if (bits & PS_VELOCITY) {
        read_int16_v(p->s.velocity, 3);
    }
    if (bits & PS_TIME) {
        p->s.pm_time = read_uint8();
    }
    if (bits & PS_FLAGS) {
        p->s.pm_flags = read_uint8();
    }
    if (bits & PS_GRAVITY) {
        p->s.pm_gravity = read_int16();
    }
    if (bits & PS_DELTA_ANGLES) {
        read_int16_v(p->s.delta_angles, 3);
    }
    if (bits & PS_VIEWOFFSET) {
        read_int8_v(p->s.viewoffset, 3);
    }
    if (bits & PS_VIEWANGLES) {
        read_int16_v(p->s.viewangles, 3);
    }
    if (bits & PS_KICKANGLES) {
        read_int8_v(p->s.kickangles, 3);
    }
    if (bits & PS_WEAPONINDEX) {
        p->s.weaponindex = read_uint8();
    }
    if (bits & PS_WEAPONFRAME) {
        p->s.weaponframe = read_uint8();
        read_int8_v(p->s.gunoffset, 3);
        read_int8_v(p->s.gunangles, 3);
    }
    if (bits & PS_BLEND) {
        read_uint8_v(p->s.blend, 4);
    }
    if (bits & PS_FOV) {
        p->s.fov = read_uint8();
    }
    if (bits & PS_RDFLAGS) {
        p->s.rdflags = read_uint8();
    }

    p->statbits = read_uint32();
    if (p->statbits) {
        for (i = 0; i < MAX_STATS; i++) {
            if (p->statbits & (1U << i)) {
                p->s.stats[i] = read_int16();
            }
        }
    }

    return NODE(p);
}

static node_t *parse_svc_frame(void)
{
    frame_t *f = alloc_node(NODE_FRAME, sizeof(*f));

    f->number = read_uint32();
    f->delta = read_uint32();
    if (current->protocol == PROTOCOL_VERSION_OLD) {
        f->suppressed = 0;
    } else {
        f->suppressed = read_uint8();
    }
    f->portalbits = parse_blob();
    if (read_uint8() != svc_playerinfo) {
        fatal("not playerinfo");
    }
    f->players = parse_svc_player();
    if (read_uint8() != svc_packetentities) {
        fatal("not packetentities");
    }
    f->entities = build_list(parse_entity);

    return NODE(f);
}

static node_t *parse_dm2_message(void)
{
    unsigned cmd;
    node_t *n;

    if (msg.head >= msg.tail)
        return NULL;

    cmd = read_uint8();

    switch (cmd) {
    case svc_serverdata:
        n = parse_svc_serverdata();
        break;
    case svc_frame:
        n = parse_svc_frame();
        break;
    case svc_spawnbaseline:
        if ((n = parse_entity()) == NULL) {
            fatal("bad entity number");
        }
        break;
    default:
        n = _parse_svc(cmd);
        break;
    }

    return n;
}

uint8_t *load_bin(demo_t *demo, size_t *size_p)
{
    size_t msglen;

    if (!demo->blocknum) {
        uint32_t magic;

        read_raw(&magic, sizeof(magic), demo->fp);
        if (magic != MVD_MAGIC) {
            if (magic == (uint32_t)-1) {
                return NULL;
            }
            msglen = le32(magic);
            demo->mode |= MODE_DM2;
            goto done;
        }
    }

    if (demo->mode & MODE_DM2) {
        uint32_t v;

        read_raw(&v, sizeof(v), demo->fp);
        if (v == (uint32_t)-1) {
            return NULL;
        }
        msglen = le32(v);
    } else {
        uint16_t v;

        read_raw(&v, sizeof(v), demo->fp);
        if (!v) {
            return NULL;
        }
        msglen = le16(v);
    }

done:
    if (msglen > MAX_MSGLEN) {
        fatal("msglen > MAX_MSGLEN");
    }

    read_raw(msg.data, msglen, demo->fp);
    msg.head = 0;
    msg.tail = msglen;

    if (size_p) {
        *size_p = msglen;
    }

    return msg.data;
}

node_t *read_bin(demo_t *demo)
{
    current = demo;

    if (load_bin(demo, NULL)) {
        if (demo->mode & MODE_DM2) {
            if (msg.tail == 0)
                return alloc_node(NODE_DUMMY, sizeof(node_t));
            return build_list(parse_dm2_message);
        } else {
            return build_list(parse_mvd_message);
        }
    } else {
        return NULL;
    }
}


//
// writing
//

static void *get_space(size_t len)
{
    void *p;

    if (len > msg.tail - msg.head) {
        fatal("message overflowed");
    }

    p = msg.data + msg.head;
    msg.head += len;
    return p;
}

static void write_uint8(unsigned c)
{
    uint8_t *p = get_space(1);

    *p = c & 255;
}

#define write_int16(c) write_uint16((unsigned)c)

static void write_uint16(unsigned c)
{
    uint8_t *p = get_space(2);

    p[0] = c & 255;
    p[1] = (c >> 8) & 255;
}

static void write_uint32(unsigned c)
{
    uint8_t *p = get_space(4);

    p[0] = c & 255;
    p[1] = (c >> 8) & 255;
    p[2] = (c >> 16) & 255;
    p[3] = (c >> 24) & 255;
}

static void write_uint8_v(unsigned *v, size_t count)
{
    uint8_t *p = get_space(count);
    size_t i;

    for (i = 0; i < count; i++) {
        p[i] = v[i];
    }
}

static void write_int8_v(int *v, size_t count)
{
    uint8_t *p = get_space(count);
    size_t i;

    for (i = 0; i < count; i++) {
        ((char *)p)[i] = v[i];
    }
}

static void write_int16_v(int *v, size_t count)
{
    size_t i;

    for (i = 0; i < count; i++) {
        write_int16(v[i]);
    }
}

static void write_data(const void *v, size_t len)
{
    memcpy(get_space(len), v, len);
}

static void write_string(const char *s)
{
    if (s) {
        write_data(s, strlen(s) + 1);
    } else {
        write_uint8(0);
    }
}

static void write_stats(player_t *p)
{
    int i;

    write_uint32(p->statbits);
    for (i = 0; i < MAX_STATS; i++) {
        if (p->statbits & (1U << i)) {
            write_int16(p->s.stats[i]);
        }
    }
}

static void write_player(player_t *p)
{
    write_uint8(p->number);
    write_uint16(p->bits);
    if (p->bits & P_TYPE) write_uint8(p->s.pm_type);
    if (p->bits & P_ORIGIN) write_int16_v(p->s.origin, 2);
    if (p->bits & P_ORIGIN2) write_int16(p->s.origin[2]);
    if (p->bits & P_VIEWOFFSET) write_int8_v(p->s.viewoffset, 3);
    if (p->bits & P_VIEWANGLES) write_int16_v(p->s.viewangles, 2);
    if (p->bits & P_VIEWANGLE2) write_int16(p->s.viewangles[2]);
    if (p->bits & P_KICKANGLES) write_int8_v(p->s.kickangles, 3);
    if (p->bits & P_WEAPONINDEX) write_uint8(p->s.weaponindex);
    if (p->bits & P_WEAPONFRAME) write_uint8(p->s.weaponframe);
    if (p->bits & P_GUNOFFSET) write_int8_v(p->s.gunoffset, 3);
    if (p->bits & P_GUNANGLES) write_int8_v(p->s.gunangles, 3);
    if (p->bits & P_BLEND) write_uint8_v(p->s.blend, 4);
    if (p->bits & P_FOV) write_uint8(p->s.fov);
    if (p->bits & P_RDFLAGS) write_uint8(p->s.rdflags);
    if (p->bits & P_STATS) write_stats(p);
}

static unsigned extend_entity(const entity_t *e)
{
    unsigned bits = e->bits & ~E_EXTENDED;
    unsigned mask = 0xffff0000;

    if (current->mode & MODE_DM2) {
        mask |= 0x8000;
    }

    if (e->number & 0xff00) {
        bits |= E_NUMBER16;
    }

    if (e->bits & E_FRAME32) {
        if (e->s.frame & 0xff00) {
            bits |= E_FRAME16;
        } else {
            bits |= E_FRAME8;
        }
    }
    if (e->bits & E_SKIN32) {
        if (e->s.skin & mask) {
            bits |= E_SKIN32;
        } else if (e->s.skin & 0x0000ff00) {
            bits |= E_SKIN16;
        } else {
            bits |= E_SKIN8;
        }
    }
    if (e->bits & E_EFFECTS32) {
        if (e->s.effects & mask) {
            bits |= E_EFFECTS32;
        } else if (e->s.effects & 0x0000ff00) {
            bits |= E_EFFECTS16;
        } else {
            bits |= E_EFFECTS8;
        }
    }
    if (e->bits & E_RENDERFX32) {
        if (e->s.renderfx & mask) {
            bits |= E_RENDERFX32;
        } else if (e->s.renderfx & 0x0000ff00) {
            bits |= E_RENDERFX16;
        } else {
            bits |= E_RENDERFX8;
        }
    }

    if (bits & 0xff000000) {
        bits |= E_MOREBITS3 | E_MOREBITS2 | E_MOREBITS1;
    } else if (bits & 0x00ff0000) {
        bits |= E_MOREBITS2 | E_MOREBITS1;
    } else if (bits & 0x0000ff00) {
        bits |= E_MOREBITS1;
    }

    return bits;
}

static void write_entity(entity_t *e)
{
    unsigned bits = extend_entity(e);

    write_uint8(bits & 255);
    if (bits & 0xff000000) {
        write_uint8((bits >> 8) & 255);
        write_uint8((bits >> 16) & 255);
        write_uint8((bits >> 24) & 255);
    } else if (bits & 0x00ff0000) {
        write_uint8((bits >> 8) & 255);
        write_uint8((bits >> 16) & 255);
    } else if (bits & 0x0000ff00) {
        write_uint8((bits >> 8) & 255);
    }

    if (bits & E_NUMBER16) {
        write_uint16(e->number);
    } else {
        write_uint8(e->number);
    }
    if (bits & E_MODEL) write_uint8(e->s.modelindex);
    if (bits & E_MODEL2) write_uint8(e->s.modelindex2);
    if (bits & E_MODEL3) write_uint8(e->s.modelindex3);
    if (bits & E_MODEL4) write_uint8(e->s.modelindex4);
    switch (bits & E_FRAME32) {
    case E_FRAME32:
        fatal("E_FRAME32 is not supported");
    case E_FRAME16:
        write_uint16(e->s.frame);
        break;
    case E_FRAME8:
        write_uint8(e->s.frame);
        break;
    }
    switch (bits & E_SKIN32) {
    case E_SKIN32:
        write_uint32(e->s.skin);
        break;
    case E_SKIN16:
        write_uint16(e->s.skin);
        break;
    case E_SKIN8:
        write_uint8(e->s.skin);
        break;
    }
    switch (bits & E_EFFECTS32) {
    case E_EFFECTS32:
        write_uint32(e->s.effects);
        break;
    case E_EFFECTS16:
        write_uint16(e->s.effects);
        break;
    case E_EFFECTS8:
        write_uint8(e->s.effects);
        break;
    }
    switch (bits & E_RENDERFX32) {
    case E_RENDERFX32:
        write_uint32(e->s.renderfx);
        break;
    case E_RENDERFX16:
        write_uint16(e->s.renderfx);
        break;
    case E_RENDERFX8:
        write_uint8(e->s.renderfx);
        break;
    }
    if (bits & E_ORIGIN1) write_int16(e->s.origin_x);
    if (bits & E_ORIGIN2) write_int16(e->s.origin_y);
    if (bits & E_ORIGIN3) write_int16(e->s.origin_z);
    if (bits & E_ANGLE1) write_uint8(e->s.angle_x);
    if (bits & E_ANGLE2) write_uint8(e->s.angle_y);
    if (bits & E_ANGLE3) write_uint8(e->s.angle_z);
    if (bits & E_OLDORIGIN) write_int16_v(e->s.old_origin, 3);
    if (bits & E_SOUND) write_uint8(e->s.sound);
    if (bits & E_EVENT) write_uint8(e->s.event);
    if (bits & E_SOLID) write_uint16(e->s.solid);
}

static void write_blob(blob_t *b)
{
    if (!b) {
        write_uint8(0);
        return;
    }
    write_uint8(b->len);
    write_data(b->data, b->len);
}

static void _write_frame(frame_t *f)
{
    write_blob((blob_t *)f->portalbits);
    iter_list(f->players, write_player);
    write_uint8(CLIENTNUM_NONE);
    iter_list(f->entities, write_entity);
    write_uint16(0);
}

static void write_frame(frame_t *f)
{
    write_uint8(mvd_frame);
    _write_frame(f);
}

static void _write_configstring(string_t *s)
{
    write_uint16(s->index);
    write_string(s->data);
}

static void write_configstring(string_t *s, unsigned cmd)
{
    write_uint8(cmd);
    _write_configstring(s);
}

static void write_gamestate(game_state_t *g)
{
    write_uint8(mvd_serverdata | (g->mvdflags << SVCMD_BITS));
    write_uint32(g->majorversion);
    write_uint16(g->minorversion);
    write_uint32(g->servercount);
    write_string(g->gamedir);
    write_int16(g->clientnum);
    iter_list(g->configstrings, _write_configstring);
    write_uint16(MAX_CONFIGSTRINGS);
    _write_frame((frame_t *)g->baseframe);
}

static void write_svc_string(string_t *s, unsigned cmd)
{
    write_uint8(cmd);
    write_string(s->data);
}

static void write_print(string_t *s, unsigned cmd)
{
    write_uint8(cmd);
    write_uint8(s->index);
    write_string(s->data);
}

static void write_muzzleflash(muzzleflash_t *m)
{
    write_uint8(svc_muzzleflash + m->type);
    write_uint16(m->entity);
    write_uint8(m->weapon);
}

static void write_svc_sound(sound_t *s)
{
    write_uint8(svc_sound);
    write_uint8(s->bits);
    write_uint8(s->index);
    if (s->bits & SND_VOLUME) write_uint8(s->volume);
    if (s->bits & SND_ATTENUATION) write_uint8(s->attenuation);
    if (s->bits & SND_OFFSET) write_uint8(s->offset);
    if (s->bits & SND_ENT) write_uint16((s->entity << 3) | s->channel);
    if (s->bits & SND_POS) write_int16_v(s->pos, 3);
}

static void write_temp_entity(tent_t *t)
{
    write_uint8(svc_temp_entity);
    write_uint8(t->type);
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
        write_int16_v(t->pos1, 3);
        write_uint8(t->dir);
        break;

    case TE_SPLASH:
    case TE_LASER_SPARKS:
    case TE_WELDING_SPARKS:
    case TE_TUNNEL_SPARKS:
        write_uint8(t->count);
        write_int16_v(t->pos1, 3);
        write_uint8(t->dir);
        write_uint8(t->color);
        break;

    case TE_BLUEHYPERBLASTER:
    case TE_RAILTRAIL:
    case TE_BUBBLETRAIL:
    case TE_DEBUGTRAIL:
    case TE_BUBBLETRAIL2:
    case TE_BFG_LASER:
        write_int16_v(t->pos1, 3);
        write_int16_v(t->pos2, 3);
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
        write_int16_v(t->pos1, 3);
        break;

    case TE_PARASITE_ATTACK:
    case TE_MEDIC_CABLE_ATTACK:
    case TE_HEATBEAM:
    case TE_MONSTER_HEATBEAM:
        write_uint16(t->entity1);
        write_int16_v(t->pos1, 3);
        write_int16_v(t->pos2, 3);
        break;

    case TE_GRAPPLE_CABLE:
        write_uint16(t->entity1);
        write_int16_v(t->pos1, 3);
        write_int16_v(t->pos2, 3);
        write_int16_v(t->offset, 3);
        break;

    case TE_LIGHTNING:
        write_uint16(t->entity1);
        write_uint16(t->entity2);
        write_int16_v(t->pos1, 3);
        write_int16_v(t->pos2, 3);
        break;

    case TE_FLASHLIGHT:
        write_int16_v(t->pos1, 3);
        write_uint16(t->entity1);
        break;

    case TE_FORCEWALL:
        write_int16_v(t->pos1, 3);
        write_int16_v(t->pos2, 3);
        write_uint8(t->color);
        break;

    case TE_STEAM:
        write_uint16(t->entity1);
        write_uint8(t->count);
        write_int16_v(t->pos1, 3);
        write_uint8(t->dir);
        write_uint8(t->color);
        write_uint16(t->entity2);
        if (t->entity1 != 0xffff) {
            write_uint32(t->time);
        }
        break;

    case TE_WIDOWBEAMOUT:
        write_uint16(t->entity1);
        write_int16_v(t->pos1, 3);
        break;

    default:
        fatal("bad temp entity type");
    }
}

static void write_inventory(inventory_t *n)
{
    write_uint8(svc_inventory);
    write_int16_v(n->inventory, MAX_ITEMS);
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
        write_svc_string(n, svc_layout);
        break;
    case NODE_STUFFTEXT:
        write_svc_string(n, svc_stufftext);
        break;
    case NODE_CENTERPRINT:
        write_svc_string(n, svc_centerprint);
        break;
    case NODE_INVENTORY:
        write_inventory(n);
        break;
    case NODE_CONFIGSTRING:
        write_configstring(n, svc_configstring);
        break;
    case NODE_SOUND:
        write_svc_sound(n);
        break;
    case NODE_PRINT:
        write_print(n, svc_print);
        break;
    case NODE_NOP:
        write_uint8(svc_nop);
        break;
    default:
        fatal("bad node type");
    }
}

static void write_svc_hdr(uint8_t *hdr, unsigned cmd, node_t *data)
{
    size_t len, pos;
    unsigned bits;

    pos = msg.head;
    iter_list(data, write_svc);
    len = msg.head - pos;

    bits = (len >> 8) & 7;
    hdr[0] = cmd | (bits << SVCMD_BITS);
    hdr[1] = len & 255;
}

static void write_unicast(unicast_t *u)
{
    uint8_t *hdr = get_space(2);
    unsigned cmd = mvd_unicast + u->reliable;

    write_uint8(u->clientnum);
    write_svc_hdr(hdr, cmd, u->data);
}

static void write_multicast(multicast_t *m)
{
    uint8_t *hdr = get_space(2);
    unsigned cmd = mvd_multicast_all + m->type + m->reliable * 3;

    if (m->type) {
        write_uint16(m->leafnum);
    }
    write_svc_hdr(hdr, cmd, m->data);
}

static void write_mvd_sound(sound_t *s)
{
    write_uint8(mvd_sound | (s->flags << SVCMD_BITS));
    write_uint8(s->bits & ~SND_POS);   // strip SND_POS, it is never used
    write_uint8(s->index);
    if (s->bits & SND_VOLUME) write_uint8(s->volume);
    if (s->bits & SND_ATTENUATION) write_uint8(s->attenuation);
    if (s->bits & SND_OFFSET) write_uint8(s->offset);
    write_uint16((s->entity << 3) | s->channel);
}

static void write_mvd_node(void *n)
{
    switch (((node_t *)n)->type) {
    case NODE_GAMESTATE:
        write_gamestate(n);
        break;
    case NODE_MULTICAST:
        write_multicast(n);
        break;
    case NODE_UNICAST:
        write_unicast(n);
        break;
    case NODE_CONFIGSTRING:
        write_configstring(n, mvd_configstring);
        break;
    case NODE_FRAME:
        write_frame(n);
        break;
    case NODE_SOUND:
        write_mvd_sound(n);
        break;
    case NODE_PRINT:
        write_print(n, mvd_print);
        break;
    case NODE_NOP:
        write_uint8(mvd_nop);
        break;
    default:
        fatal("bad node type");
    }
}

static void write_svc_serverdata(serverdata_t *s)
{
    write_uint8(svc_serverdata);
    write_uint32(s->majorversion);
    write_uint32(s->servercount);
    write_uint8(s->attractloop);
    write_string(s->gamedir);
    write_int16(s->clientnum);
    write_string(s->levelname);
    current->protocol = s->majorversion;
}

static void write_svc_player(player_t *p)
{
    if (!p) {
        write_uint16(0);
        write_uint32(0);
        return;
    }
    write_uint16(p->bits);
    if (p->bits & PS_TYPE) write_uint8(p->s.pm_type);
    if (p->bits & PS_ORIGIN) write_int16_v(p->s.origin, 3);
    if (p->bits & PS_VELOCITY) write_int16_v(p->s.velocity, 3);
    if (p->bits & PS_TIME) write_uint8(p->s.pm_time);
    if (p->bits & PS_FLAGS) write_uint8(p->s.pm_flags);
    if (p->bits & PS_GRAVITY) write_int16(p->s.pm_gravity);
    if (p->bits & PS_DELTA_ANGLES) write_int16_v(p->s.delta_angles, 3);
    if (p->bits & PS_VIEWOFFSET) write_int8_v(p->s.viewoffset, 3);
    if (p->bits & PS_VIEWANGLES) write_int16_v(p->s.viewangles, 3);
    if (p->bits & PS_KICKANGLES) write_int8_v(p->s.kickangles, 3);
    if (p->bits & PS_WEAPONINDEX) write_uint8(p->s.weaponindex);
    if (p->bits & PS_WEAPONFRAME) {
        write_uint8(p->s.weaponframe);
        write_int8_v(p->s.gunoffset, 3);
        write_int8_v(p->s.gunangles, 3);
    }
    if (p->bits & PS_BLEND) write_uint8_v(p->s.blend, 4);
    if (p->bits & PS_FOV) write_uint8(p->s.fov);
    if (p->bits & PS_RDFLAGS) write_uint8(p->s.rdflags);
    write_stats(p);
}

static void write_svc_frame(frame_t *f)
{
    write_uint8(svc_frame);
    write_uint32(f->number);
    write_uint32(f->delta);
    if (current->protocol != PROTOCOL_VERSION_OLD)
        write_uint8(f->suppressed);
    write_blob((blob_t *)f->portalbits);
    write_uint8(svc_playerinfo);
    write_svc_player((player_t *)f->players);
    write_uint8(svc_packetentities);
    iter_list(f->entities, write_entity);
    write_uint16(0);
}

static void write_dm2_node(void *n)
{
    switch (((node_t *)n)->type) {
    case NODE_DUMMY:
        break;
    case NODE_SERVERDATA:
        write_svc_serverdata(n);
        break;
    case NODE_FRAME:
        write_svc_frame(n);
        break;
    case NODE_ENTITY:
        write_uint8(svc_spawnbaseline);
        write_entity(n);
        break;
    default:
        write_svc(n);
        break;
    }
}

size_t write_bin(demo_t *demo, node_t *nodes)
{
    current = demo;

    msg.head = 0;
    msg.tail = MAX_MSGLEN;

    if (demo->mode & MODE_DM2) {
        iter_list(nodes, write_dm2_node);

        uint32_t v = le32(msg.head);
        write_raw(&v, sizeof(v), demo->fp);
    } else {
        iter_list(nodes, write_mvd_node);

        if (!demo->blocknum) {
            uint32_t v = MVD_MAGIC;
            write_raw(&v, sizeof(v), demo->fp);
        }

        if (msg.head) {
            uint16_t v = le16(msg.head);
            write_raw(&v, sizeof(v), demo->fp);
        }
    }

    write_raw(msg.data, msg.head, demo->fp);

    return msg.head;
}
