
#define MAX_MSGLEN          0x8000
#define MAX_PACKETLEN       1400    // max default Q2 UDP packet length
#define MAX_STATS           32
#define MAX_QPATH           64
#define MAX_STRING_CHARS    1024
#define MAX_NET_STRING      2048
#define MAX_CONFIGSTRINGS   2080
#define MAX_CLIENTS         256
#define MAX_EDICTS          1024
#define MAX_ITEMS           256

#define PROTOCOL_VERSION_OLD            26
#define PROTOCOL_VERSION_DEFAULT        34
#define PROTOCOL_VERSION_MVD            37

#define PROTOCOL_VERSION_MVD_MINIMUM    2009    // r168
#define PROTOCOL_VERSION_MVD_CURRENT    2010    // r177

#define MVD_SUPPORTED(x) \
    ((x)>=PROTOCOL_VERSION_MVD_MINIMUM && \
     (x)<=PROTOCOL_VERSION_MVD_CURRENT)

#define SVCMD_BITS          5
#define SVCMD_MASK          ((1<<SVCMD_BITS)-1)

#define MVD_MAGIC           mkle32('M','V','D','2')

#define CLIENTNUM_NONE      (MAX_CLIENTS-1)

#define F(x)    (1<<(x))

// Q2 protocol sepcific opertaions
typedef enum {
    svc_bad,
    svc_muzzleflash,
    svc_muzzleflash2,
    svc_temp_entity,
    svc_layout,
    svc_inventory,
    svc_nop,
    svc_disconnect,
    svc_reconnect,
    svc_sound,
    svc_print,
    svc_stufftext,
    svc_serverdata,
    svc_configstring,
    svc_spawnbaseline,
    svc_centerprint,
    svc_download,
    svc_playerinfo,
    svc_packetentities,
    svc_deltapacketentities,
    svc_frame,
    svc_num_types
} svc_ops_t;

// MVD protocol specific operations
typedef enum {
    mvd_bad,
    mvd_nop,
    mvd_disconnect,     // reserved
    mvd_reconnect,      // reserved
    mvd_serverdata,
    mvd_configstring,
    mvd_frame,
    mvd_frame_nodelta,  // reserved
    mvd_unicast,
    mvd_unicast_r,
    mvd_multicast_all,
    mvd_multicast_phs,
    mvd_multicast_pvs,
    mvd_multicast_all_r,
    mvd_multicast_phs_r,
    mvd_multicast_pvs_r,
    mvd_sound,
    mvd_print,
    mvd_stufftext,      // reserved
    mvd_num_types
} mvd_ops_t;

// MVD stream flags (only 3 bits can be used)
typedef enum {
    MVF_NOMSGS      = F(0),
    MVF_RESERVED1   = F(1),
    MVF_RESERVED2   = F(2)
} mvd_flags_t;

//
// players
//
typedef enum {
    P_TYPE          = F(0),
    P_ORIGIN        = F(1),
    P_ORIGIN2       = F(2),
    P_VIEWOFFSET    = F(3),
    P_VIEWANGLES    = F(4),
    P_VIEWANGLE2    = F(5),
    P_KICKANGLES    = F(6),
    P_BLEND         = F(7),
    P_FOV           = F(8),
    P_WEAPONINDEX   = F(9),
    P_WEAPONFRAME   = F(10),
    P_GUNOFFSET     = F(11),
    P_GUNANGLES     = F(12),
    P_RDFLAGS       = F(13),
    P_STATS         = F(14),
    P_REMOVE        = F(15)
} player_bits_t;

typedef enum {
    PS_TYPE         = F(0),
    PS_ORIGIN       = F(1),
    PS_VELOCITY     = F(2),
    PS_TIME         = F(3),
    PS_FLAGS        = F(4),
    PS_GRAVITY      = F(5),
    PS_DELTA_ANGLES = F(6),
    PS_VIEWOFFSET   = F(7),
    PS_VIEWANGLES   = F(8),
    PS_KICKANGLES   = F(9),
    PS_BLEND        = F(10),
    PS_FOV          = F(11),
    PS_WEAPONINDEX  = F(12),
    PS_WEAPONFRAME  = F(13),
    PS_RDFLAGS      = F(14),
    PS_RESERVED     = F(15)
} player_svc_bits_t;

//
// entities
//
typedef enum {
    E_ORIGIN1       = F(0),
    E_ORIGIN2       = F(1),
    E_ANGLE2        = F(2),
    E_ANGLE3        = F(3),
    E_FRAME8        = F(4),
    E_EVENT         = F(5),
    E_REMOVE        = F(6),
    E_MOREBITS1     = F(7),

    E_NUMBER16      = F(8),
    E_ORIGIN3       = F(9),
    E_ANGLE1        = F(10),
    E_MODEL         = F(11),
    E_RENDERFX8     = F(12),
    // bit 13 is not used
    E_EFFECTS8      = F(14),
    E_MOREBITS2     = F(15),

    E_SKIN8         = F(16),
    E_FRAME16       = F(17),
    E_RENDERFX16    = F(18),
    E_EFFECTS16     = F(19),
    E_MODEL2        = F(20),
    E_MODEL3        = F(21),
    E_MODEL4        = F(22),
    E_MOREBITS3     = F(23),

    E_OLDORIGIN     = F(24),
    E_SKIN16        = F(25),
    E_SOUND         = F(26),
    E_SOLID         = F(27),

#define E_FRAME32       (E_FRAME8|E_FRAME16)    // never actually used
#define E_SKIN32        (E_SKIN8|E_SKIN16)
#define E_RENDERFX32    (E_RENDERFX8|E_RENDERFX16)
#define E_EFFECTS32     (E_EFFECTS8|E_EFFECTS16)
#define E_EXTENDED      (E_NUMBER16|E_FRAME32|E_SKIN32|E_EFFECTS32|\
                         E_RENDERFX32|E_MOREBITS3|E_MOREBITS2|E_MOREBITS1)
#define E_MASK          (F(28)-1)
} entity_bits_t;

//
// sounds
//

#define SND_VOLUME      F(0)
#define SND_ATTENUATION F(1)
#define SND_POS         F(2)
#define SND_ENT         F(3)
#define SND_OFFSET      F(4)

//
// temp entities
//
typedef enum {
    TE_GUNSHOT,
    TE_BLOOD,
    TE_BLASTER,
    TE_RAILTRAIL,
    TE_SHOTGUN,
    TE_EXPLOSION1,
    TE_EXPLOSION2,
    TE_ROCKET_EXPLOSION,
    TE_GRENADE_EXPLOSION,
    TE_SPARKS,
    TE_SPLASH,
    TE_BUBBLETRAIL,
    TE_SCREEN_SPARKS,
    TE_SHIELD_SPARKS,
    TE_BULLET_SPARKS,
    TE_LASER_SPARKS,
    TE_PARASITE_ATTACK,
    TE_ROCKET_EXPLOSION_WATER,
    TE_GRENADE_EXPLOSION_WATER,
    TE_MEDIC_CABLE_ATTACK,
    TE_BFG_EXPLOSION,
    TE_BFG_BIGEXPLOSION,
    TE_BOSSTPORT,
    TE_BFG_LASER,
    TE_GRAPPLE_CABLE,
    TE_WELDING_SPARKS,
    TE_GREENBLOOD,
    TE_BLUEHYPERBLASTER,
    TE_PLASMA_EXPLOSION,
    TE_TUNNEL_SPARKS,
    TE_BLASTER2,
    TE_RAILTRAIL2,
    TE_FLAME,
    TE_LIGHTNING,
    TE_DEBUGTRAIL,
    TE_PLAIN_EXPLOSION,
    TE_FLASHLIGHT,
    TE_FORCEWALL,
    TE_HEATBEAM,
    TE_MONSTER_HEATBEAM,
    TE_STEAM,
    TE_BUBBLETRAIL2,
    TE_MOREBLOOD,
    TE_HEATBEAM_SPARKS,
    TE_HEATBEAM_STEAM,
    TE_CHAINFIST_SMOKE,
    TE_ELECTRIC_SPARKS,
    TE_TRACKER_EXPLOSION,
    TE_TELEPORT_EFFECT,
    TE_DBALL_GOAL,
    TE_WIDOWBEAMOUT,
    TE_NUKEBLAST,
    TE_WIDOWSPLASH,
    TE_EXPLOSION1_BIG,
    TE_EXPLOSION1_NP,
    TE_FLECHETTE,

    TE_NUM_ENTITIES
} temp_event_t;

//
// config strings
//

typedef enum {
    CS_NAME,
    CS_CDTRACK,
    CS_SKY,
    CS_SKYAXIS,
    CS_SKYROTATE,
    CS_STATUSBAR,
    CS_AIRACCEL = 29,
    CS_MAXCLIENTS,
    CS_MAPCHECKSUM,
    CS_MODELS,

} cs_index_t;

// HACK: some mods exploit CS_STATUSBAR to take space up to CS_AIRACCEL
#define CS_SIZE(cs) \
    ((cs)>=CS_STATUSBAR&&(cs)<CS_AIRACCEL?MAX_QPATH*(CS_AIRACCEL-(cs)):MAX_QPATH)

//
// print levels
//

typedef enum {
    PRINT_LOW,
    PRINT_MEDIUM,
    PRINT_HIGH,
    PRINT_CHAT
} print_level_t;
