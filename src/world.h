typedef struct {
    bool active;
    unsigned remove;
    entity_state_t s;
} world_entity_t;

typedef struct {
    bool active;
    unsigned remove;
    player_state_t s;
} world_player_t;

typedef struct {
    unsigned mvdflags;
    unsigned servercount;
    int clientnum;
    char gamedir[MAX_QPATH];
    char configstrings[MAX_CONFIGSTRINGS][MAX_QPATH + 1];
    world_entity_t entities[MAX_EDICTS];
    world_player_t players[MAX_CLIENTS];
    uint8_t portalbits[256];
    unsigned portalbytes;
} world_state_t;

node_t *world_delta(world_state_t *from, world_state_t *to);
bool update_world(world_state_t *world, void *n);
