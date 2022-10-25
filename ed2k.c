#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MD4_DIGEST_SIZE        16
#define MD4_HMAC_BLOCK_SIZE    64
#define MD4_BLOCK_WORDS        16
#define MD4_HASH_WORDS         4

struct md4_ctx {
    uint32_t hash[MD4_HASH_WORDS];
    uint32_t block[MD4_BLOCK_WORDS];
    uint64_t byte_count;
};

static inline uint32_t lshift(uint32_t x, unsigned int s)
{
    x &= 0xFFFFFFFF;
    return ((x << s) & 0xFFFFFFFF) | (x >> (32 - s));
}

static inline uint32_t F(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) | ((~x) & z);
}

static inline uint32_t G(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) | (x & z) | (y & z);
}

static inline uint32_t H(uint32_t x, uint32_t y, uint32_t z)
{
    return x ^ y ^ z;
}

#define ROUND1(a,b,c,d,k,s) (a = lshift(a + F(b,c,d) + k, s))
#define ROUND2(a,b,c,d,k,s) (a = lshift(a + G(b,c,d) + k + (uint32_t)0x5A827999,s))
#define ROUND3(a,b,c,d,k,s) (a = lshift(a + H(b,c,d) + k + (uint32_t)0x6ED9EBA1,s))

static void md4_transform(uint32_t *hash, uint32_t const *in)
{
    uint32_t a, b, c, d;

    a = hash[0];
    b = hash[1];
    c = hash[2];
    d = hash[3];

    ROUND1(a, b, c, d, in[0], 3);
    ROUND1(d, a, b, c, in[1], 7);
    ROUND1(c, d, a, b, in[2], 11);
    ROUND1(b, c, d, a, in[3], 19);
    ROUND1(a, b, c, d, in[4], 3);
    ROUND1(d, a, b, c, in[5], 7);
    ROUND1(c, d, a, b, in[6], 11);
    ROUND1(b, c, d, a, in[7], 19);
    ROUND1(a, b, c, d, in[8], 3);
    ROUND1(d, a, b, c, in[9], 7);
    ROUND1(c, d, a, b, in[10], 11);
    ROUND1(b, c, d, a, in[11], 19);
    ROUND1(a, b, c, d, in[12], 3);
    ROUND1(d, a, b, c, in[13], 7);
    ROUND1(c, d, a, b, in[14], 11);
    ROUND1(b, c, d, a, in[15], 19);

    ROUND2(a, b, c, d,in[ 0], 3);
    ROUND2(d, a, b, c, in[4], 5);
    ROUND2(c, d, a, b, in[8], 9);
    ROUND2(b, c, d, a, in[12], 13);
    ROUND2(a, b, c, d, in[1], 3);
    ROUND2(d, a, b, c, in[5], 5);
    ROUND2(c, d, a, b, in[9], 9);
    ROUND2(b, c, d, a, in[13], 13);
    ROUND2(a, b, c, d, in[2], 3);
    ROUND2(d, a, b, c, in[6], 5);
    ROUND2(c, d, a, b, in[10], 9);
    ROUND2(b, c, d, a, in[14], 13);
    ROUND2(a, b, c, d, in[3], 3);
    ROUND2(d, a, b, c, in[7], 5);
    ROUND2(c, d, a, b, in[11], 9);
    ROUND2(b, c, d, a, in[15], 13);

    ROUND3(a, b, c, d,in[ 0], 3);
    ROUND3(d, a, b, c, in[8], 9);
    ROUND3(c, d, a, b, in[4], 11);
    ROUND3(b, c, d, a, in[12], 15);
    ROUND3(a, b, c, d, in[2], 3);
    ROUND3(d, a, b, c, in[10], 9);
    ROUND3(c, d, a, b, in[6], 11);
    ROUND3(b, c, d, a, in[14], 15);
    ROUND3(a, b, c, d, in[1], 3);
    ROUND3(d, a, b, c, in[9], 9);
    ROUND3(c, d, a, b, in[5], 11);
    ROUND3(b, c, d, a, in[13], 15);
    ROUND3(a, b, c, d, in[3], 3);
    ROUND3(d, a, b, c, in[11], 9);
    ROUND3(c, d, a, b, in[7], 11);
    ROUND3(b, c, d, a, in[15], 15);

    hash[0] += a;
    hash[1] += b;
    hash[2] += c;
    hash[3] += d;
}

static int md4_init(struct md4_ctx *mctx)
{
    mctx->hash[0] = 0x67452301;
    mctx->hash[1] = 0xefcdab89;
    mctx->hash[2] = 0x98badcfe;
    mctx->hash[3] = 0x10325476;
    mctx->byte_count = 0;

    return 0;
}

static int md4_update(struct md4_ctx *mctx, const uint8_t *data, unsigned int len)
{
    const uint32_t avail = sizeof(mctx->block) - (mctx->byte_count & 0x3f);

    mctx->byte_count += len;

    if (avail > len) {
        memcpy((char *)mctx->block + (sizeof(mctx->block) - avail),
               data, len);
        return 0;
    }

    memcpy((char *)mctx->block + (sizeof(mctx->block) - avail),
           data, avail);

    md4_transform(mctx->hash, mctx->block);
    data += avail;
    len -= avail;

    while (len >= sizeof(mctx->block)) {
        memcpy(mctx->block, data, sizeof(mctx->block));
        md4_transform(mctx->hash, mctx->block);
        data += sizeof(mctx->block);
        len -= sizeof(mctx->block);
    }

    memcpy(mctx->block, data, len);

    return 0;
}

static int md4_final(struct md4_ctx *mctx, uint8_t *out)
{
    const unsigned int offset = mctx->byte_count & 0x3f;
    char *p = (char *)mctx->block + offset;
    int padding = 56 - (offset + 1);

    *p++ = 0x80;
    if (padding < 0) {
        memset(p, 0x00, padding + sizeof (uint64_t));
        md4_transform(mctx->hash, mctx->block);
        p = (char *)mctx->block;
        padding = 56;
    }

    memset(p, 0, padding);
    mctx->block[14] = mctx->byte_count << 3;
    mctx->block[15] = mctx->byte_count >> 29;
    md4_transform(mctx->hash, mctx->block);
    memcpy(out, mctx->hash, sizeof(mctx->hash));
    memset(mctx, 0, sizeof(*mctx));

    return 0;
}

#define ED2K_CHUNK_SIZE 9728000
#define ED2K_BUF_SIZE   8000

#if defined(__EMSCRIPTEN__)
#include "emscripten.h"
#define EXPORT EMSCRIPTEN_KEEPALIVE
#define ED2K_LIBRARY
#else
#define EXPORT
#endif

#if defined(ED2K_LIBRARY)
EXPORT void ed2k(const char *path, char *out) {
#else
static void ed2k(const char *path) {
#endif
        FILE* fh = fopen(path, "rb");
        if (!fh) {
            printf("ERROR: Failed to open \"%s\"\n", path);
            return;
        }
        
        fseek(fh, 0L, SEEK_END);
        size_t fh_size = ftell(fh);
        rewind(fh);
        int too_small = (fh_size < ED2K_CHUNK_SIZE);
        
        unsigned char buf[ED2K_BUF_SIZE], md[MD4_DIGEST_SIZE];
        struct md4_ctx root, chunk;
        md4_init(&root);
        md4_init(&chunk);
        
        size_t cur_len   = 0,
        len       = 0,
        cur_chunk = 0,
        cur_buf   = 0;
        while (fread(buf, sizeof(*buf), ED2K_BUF_SIZE, fh) > 0) {
            len        = ftell(fh);
            cur_buf    = len - cur_len;
            md4_update(&chunk, buf, cur_buf);
            cur_len    = len;
            cur_chunk += ED2K_BUF_SIZE;
            
            if (cur_chunk == ED2K_CHUNK_SIZE && cur_buf == ED2K_BUF_SIZE) {
                cur_chunk = 0;
                md4_final(&chunk, md);
                md4_init(&chunk);
                md4_update(&root, md, MD4_DIGEST_SIZE);
            }
        }
        md4_final(&chunk, md);
        fclose(fh);
        
        if (!too_small) {
            md4_update(&root, md, MD4_DIGEST_SIZE);
            md4_final(&root, md);
        }
        
        printf("ed2k://|file|%s|%lu|", path, fh_size);
        for (int i; i < MD4_DIGEST_SIZE; ++i)
            printf("%02x", md[i]);
        printf("\n");
    }
    
#if !defined(ED2K_LIBRARY)
#if defined(ED2K_ENABLE_PTHREAD)
#include <pthread.h>
    
static void* ed2k_pthread_wrapper(void *path) {
    ed2k((const char*)path);
    return NULL;
}
#endif

int main(int argc, const char *argv[]) {
#if defined(ED2K_ENABLE_PTHREAD)
    pthread_t threads[argc - 1];
    for (int i = 1; i < argc; i++)
        pthread_create(&threads[i - 1], NULL, ed2k_pthread_wrapper, (void*)argv[i]);
    for (int i = 0; i < argc - 1; i++)
        pthread_join(threads[i], NULL);
#else
    for (int i = 1; i < argc; i++)
        ed2k(argv[i]);
#endif
    return 0;
}
#endif
