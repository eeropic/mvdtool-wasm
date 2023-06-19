/* Based on Public Domain SHA-1 code by Steve Reid */

typedef struct {
    uint32_t state[5];
    uint64_t count;
    uint8_t buffer[64];
} SHA1_CTX;

void sha1_init(SHA1_CTX *ctx);
void sha1_update(SHA1_CTX *ctx, uint8_t *data, size_t len);
void sha1_final(SHA1_CTX *ctx, uint8_t digest[20]);
