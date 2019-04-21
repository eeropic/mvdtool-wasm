typedef struct node_s {
    struct node_s *next;
    enum {
        NODE_NOP,
        NODE_GAMESTATE,
        NODE_SERVERDATA,
        NODE_CONFIGSTRING,
        NODE_FRAME,
        NODE_UNICAST,
        NODE_MULTICAST,
        NODE_SOUND,
        NODE_PRINT,
        NODE_MUZZLEFLASH,
        NODE_TEMP_ENTITY,
        NODE_LAYOUT,
        NODE_STUFFTEXT,
        NODE_CENTERPRINT,
        NODE_BLOB,
        NODE_ENTITY,
        NODE_PLAYER
    } type;
    size_t start, end; // byte offsets in block, for debugging
} node_t;

#define F_PLAYER \
    DF(1,P_TYPE,pm_type); \
    DF(2,P_ORIGIN,origin); \
    DF(1,P_ORIGIN2,origin[2]); \
    DF(3,P_VIEWOFFSET,viewoffset); \
    DF(2,P_VIEWANGLES,viewangles); \
    DF(1,P_VIEWANGLE2,viewangles[2]); \
    DF(3,P_KICKANGLES,kickangles); \
    DF(1,P_WEAPONINDEX,weaponindex); \
    DF(1,P_WEAPONFRAME,weaponframe); \
    DF(3,P_GUNOFFSET,gunoffset); \
    DF(3,P_GUNANGLES,gunangles); \
    DF(4,P_BLEND,blend); \
    DF(1,P_FOV,fov); \
    DF(1,P_RDFLAGS,rdflags);

typedef struct {
    unsigned    pm_type;
    unsigned    pm_flags;
    unsigned    pm_time;
    int         pm_gravity;
    int         origin[3];
    int         velocity[3];
    int         delta_angles[3];
    int         viewoffset[3];
    int         viewangles[3];
    int         kickangles[3];
    unsigned    weaponindex;
    unsigned    weaponframe;
    int         gunoffset[3];
    int         gunangles[3];
    unsigned    blend[4];
    unsigned    fov;
    unsigned    rdflags;
    int         stats[MAX_STATS];
} player_state_t;

typedef struct {
    node_t          node;
    unsigned        bits;
    unsigned        number;
    unsigned        statbits;
    player_state_t  s;
} player_t;

#define F_ENTITY \
    DF(1,E_MODEL,modelindex); \
    DF(1,E_MODEL2,modelindex2); \
    DF(1,E_MODEL3,modelindex3); \
    DF(1,E_MODEL4,modelindex4); \
    DF(1,E_FRAME32,frame); \
    DF(1,E_SKIN32,skin); \
    DF(1,E_EFFECTS32,effects); \
    DF(1,E_RENDERFX32,renderfx); \
    DF(1,E_ORIGIN1,origin_x); \
    DF(1,E_ORIGIN2,origin_y); \
    DF(1,E_ORIGIN3,origin_z); \
    DF(1,E_ANGLE1,angle_x); \
    DF(1,E_ANGLE2,angle_y); \
    DF(1,E_ANGLE3,angle_z); \
    DF(3,E_OLDORIGIN,old_origin); \
    DF(1,E_SOUND,sound); \
    DF(1,E_EVENT,event); \
    DF(1,E_SOLID,solid);

typedef struct {
    unsigned    modelindex;
    unsigned    modelindex2;
    unsigned    modelindex3;
    unsigned    modelindex4;
    unsigned    frame;
    unsigned    skin;
    unsigned    effects;
    unsigned    renderfx;
    int         origin_x;
    int         origin_y;
    int         origin_z;
    int         angle_x;
    int         angle_y;
    int         angle_z;
    int         old_origin[3];
    unsigned    sound;
    unsigned    event;
    unsigned    solid;
} entity_state_t;

typedef struct {
    node_t          node;
    unsigned        bits;
    unsigned        number;
    entity_state_t  s;
} entity_t;

typedef struct {
    node_t node;
    size_t len;
    uint8_t data[1];
} blob_t;

typedef struct {
    node_t node;
    unsigned number;
    unsigned delta;
    unsigned suppressed;
    node_t *portalbits;
    node_t *players;
    node_t *entities;
} frame_t;

typedef struct {
    node_t node;
    unsigned index;
    size_t len;     // excluding trailing '\0'
    char data[1];
} string_t;

typedef struct {
    node_t node;
    unsigned mvdflags;
    unsigned majorversion;
    unsigned minorversion;
    unsigned servercount;
    int clientnum;
    char gamedir[MAX_QPATH];
    node_t *configstrings;
    node_t *baseframe;
} game_state_t;

typedef struct {
    node_t node;
    unsigned majorversion;
    unsigned servercount;
    unsigned attractloop;
    int clientnum;
    char gamedir[MAX_QPATH];
    char levelname[MAX_QPATH];
} serverdata_t;

typedef struct {
    node_t node;
    unsigned type;
    int pos1[3];
    int pos2[3];
    int offset[3];
    unsigned dir;
    unsigned count;
    unsigned color;
    unsigned entity1;
    unsigned entity2;
    unsigned time;
} tent_t;

typedef struct {
    node_t node;
    unsigned bits;
    unsigned flags;
    unsigned index;
    unsigned volume;
    unsigned attenuation;
    unsigned offset;
    unsigned entity;
    unsigned channel;
    int pos[3];
} sound_t;

typedef struct {
    node_t node;
    unsigned type;
    unsigned entity;
    unsigned weapon;
} muzzleflash_t;

typedef struct {
    node_t node;
    unsigned type;
    unsigned leafnum;
    bool reliable;
    node_t *data;
} multicast_t;

typedef struct {
    node_t node;
    unsigned clientnum;
    bool reliable;
    node_t *data;
} unicast_t;

#define NODE(x) &(x)->node

void *alloc_node(unsigned type, size_t size);
void free_node(void *n);
void free_nodes(void *n);

typedef node_t *(*build_func_t)(void);
node_t *build_list(build_func_t build);

typedef void (*iter_func_t)(void *);
void _iter_list(void *n, iter_func_t iter);
#define iter_list(n, iter)  _iter_list(n, (iter_func_t)iter)

