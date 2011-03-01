static inline uint16_t swap16( uint16_t x ) {
    x = ( x >> 8 ) | ( x << 8 );
    return x;
}

static inline uint32_t swap32( uint32_t x ) {
    x = ( ( x >> 8 ) & 0x00ff00ff ) | ( ( x << 8 ) & 0xff00ff00 );
    x = ( x >> 16 ) | ( x << 16 );
    return x;
}

static inline uint64_t swap64( uint64_t x ) {
    union {
        uint64_t ll;
        uint32_t l[2];
    } a, b;

    a.ll = x;
    b.l[0] = swap32( a.l[1] );
    b.l[1] = swap32( a.l[0] );

    return b.ll;
}

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define be16 swap16
#define be32 swap32
#define be64 swap64
#define le16(x) ((uint16_t)(x))
#define le32(x) ((uint32_t)(x))
#define le64(x) ((uint64_t)(x))
#define mkle16(a,b) ((a)|((b)<<8))
#define mkle32(a,b,c,d) ((a)|((b)<<8)|((c)<<16)|((d)<<24))
#elif __BYTE_ORDER == __BIG_ENDIAN
#define be16(x) ((uint16_t)(x))
#define be32(x) ((uint32_t)(x))
#define be64(x) ((uint64_t)(x))
#define le16 swap16
#define le32 swap32
#define le64 swap64
#define mkle16(a,b) ((b)|((a)<<8))
#define mkle32(a,b,c,d) ((d)|((c)<<8)|((b)<<16)|((a)<<24))
#endif
