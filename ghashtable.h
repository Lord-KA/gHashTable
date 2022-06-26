#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <nmmintrin.h>
#include <stdbool.h>

#pragma GCC target "avx2"
#pragma GCC target "sse4.2"

typedef size_t T;
#define NOT_FOUND_VAL (-1)

#ifndef MAX_KEY_LEN
#define MAX_KEY_LEN (64)
#endif

#ifndef GHT_CAPACITY
#define GHT_CAPACITY 1046527lu
#endif

typedef enum {
    EMPTY = 0,
    DELETED,
    IN_USE,
} gHT_NodeSt;

typedef struct {
    char key[MAX_KEY_LEN];
    size_t hash1;
    size_t hash2;
    size_t hash3;
    size_t hash4;
    T value;
    gHT_NodeSt state;
} gHT_Node;

typedef struct {
    gHT_Node *data;
} gHT;

static size_t gHT_hashKey(const char *key)
{
    char *iter = (char*)key;
    size_t hash = 0;
    size_t i = 0;
    while (*iter != '\0') {
        hash = _mm_crc32_u8(hash + i, *iter);
        ++iter;
        ++i;
    }
    return hash;
}

static inline size_t hash_rol_asm(size_t hash, char data)
{
    __asm__ __volatile__ (".intel_syntax noprefix;"
        "rol rbx, 1;"
        "add rbx, rcx;"
        ".att_syntax prefix;"
        : "=b" (hash)
        : "b" (hash), "c" (data)
    );
    return hash;
}

static inline size_t gHT_hashSecond(const char *key)
{
    char *iter = (char*)key;
    size_t hash = 0;
    size_t i = 0;
    while (*iter != '\0') {
        hash = hash_rol_asm(hash + i, *iter);
        ++iter;
        ++i;
    }
    return hash;
}

static inline size_t gHT_hash3(const char *key)
{

    char *iter = (char*)key;
    uint64_t hash = 0;
    const size_t FACTOR = 5;
    while (*iter != '\0') {
        hash <<= FACTOR;
        hash += *iter++;
    }
    return hash;
}

static inline size_t gHT_hash4(const char *key)
{
    char *iter = (char*)key;
    uint64_t hash = 0;
    size_t mul = 1;
    size_t mod = 1000000007;
    while (*iter != '\0') {
        size_t letter = *iter;
        hash = (hash * mul % mod + letter) % mod;
        mul *= 257;
        hash %= mod;
        ++iter;
    }
    return hash;
}

static gHT* gHT_new()
{
    gHT* ctx = (gHT*)calloc(1, sizeof(gHT));
    ctx->data = (gHT_Node*)calloc(GHT_CAPACITY, sizeof(gHT_Node));
    if (ctx->data == NULL) {
        free(ctx);
        return NULL;
    }
    return ctx;
}

static gHT* gHT_delete(gHT *ctx)
{
    if (ctx == NULL) {
        #ifndef NDEBUG
            fprintf(stderr, "WARNING: trying to delete struct by NULL ptr!\n");
        #endif
        return NULL;
    }
    free(ctx->data);
    free(ctx);
    return NULL;
}

static gHT_Node *gHT_find_internal_(gHT *ctx, char *key)
{
    assert(ctx != NULL);
    assert(GHT_CAPACITY != 0 && GHT_CAPACITY < 1<<30);

    size_t hash = gHT_hashKey(key) % GHT_CAPACITY;

    gHT_Node *iter = ctx->data + hash;
    size_t inc = gHT_hashSecond(key);
    size_t hash2 = inc;
    if (inc == 0)
        ++inc;
    size_t hash3 = gHT_hash3(key);
    size_t hash4 = gHT_hash4(key);
    for (size_t i = 0; i < GHT_CAPACITY; ++i, iter = ctx->data + (hash + i) % GHT_CAPACITY) {
        if (iter->state == EMPTY)
            return NULL;
        if (iter->state == IN_USE && iter->hash1 == hash && iter->hash2 == hash2 && iter->hash3 == hash3 && iter->hash4 == hash4 && !strcmp(key, iter->key))
            return iter;
    }

    return NULL;
}

static T gHT_find(gHT *ctx, char *key)
{
    assert(ctx != NULL);
    if (ctx == NULL)
        return NOT_FOUND_VAL;

    gHT_Node *res = gHT_find_internal_(ctx, key);
    if (res == NULL)
        return NOT_FOUND_VAL;
    return gHT_find_internal_(ctx, key)->value;
}

static T gHT_insert(gHT *ctx, char *key, size_t val, bool update)
{
    assert(ctx != NULL);
    assert(GHT_CAPACITY != 0 && GHT_CAPACITY < 1<<30);
    if (ctx == NULL)
        return NOT_FOUND_VAL;

    gHT_Node *res = gHT_find_internal_(ctx, key);
    if ((update && res == NULL) || (!update && res != NULL))
        return NOT_FOUND_VAL;

    if (update) {
        res->value = val;
        return 1;
    }

    size_t hash = gHT_hashKey(key) % GHT_CAPACITY;
    gHT_Node *iter = ctx->data + hash;
    size_t inc = gHT_hashSecond(key);
    size_t hash2 = inc;
    if (inc == 0)
        ++inc;
    size_t hash3 = gHT_hash3(key);
    size_t hash4 = gHT_hash4(key);
    for (size_t i = 0; i < GHT_CAPACITY; ++i, iter = ctx->data + (hash + i) % GHT_CAPACITY) {
        if (iter->state != IN_USE) {
            iter->hash1 = hash;
            iter->hash2 = hash2;
            iter->hash3 = hash3;
            iter->hash4 = hash4;
            iter->state = IN_USE;
            iter->value = val;
            strncpy(iter->key, key, MAX_KEY_LEN);

            return 10;
        }
    }
    assert(false);
    return NOT_FOUND_VAL;
}

static T gHT_erase(gHT *ctx, char *key)
{
    assert(ctx != NULL);
    assert(GHT_CAPACITY != 0 && GHT_CAPACITY < 1<<30);
    if (ctx == NULL)
        return NOT_FOUND_VAL;

    gHT_Node *res = gHT_find_internal_(ctx, key);
    if (res == NULL)
        return NOT_FOUND_VAL;

    res->state = DELETED;
    return 10;
}

static void gHT_dump(gHT *ctx, FILE *out)
{
    assert(ctx != NULL);
    assert(GHT_CAPACITY != 0 && GHT_CAPACITY < 1<<30);
    if (ctx == NULL)
        return;

    fprintf(out, "capacity = %zu\n", GHT_CAPACITY);
    for (size_t i = 0; i < GHT_CAPACITY; ++i) {
        if (ctx->data[i].state == IN_USE) {
            fprintf(out, "%zu \t |$%s$ | %zu | ", i, ctx->data[i].key, ctx->data[i].value);
            fprintf(out, "IN_USE");
        } else if (ctx->data[i].state == DELETED)
            fprintf(out, "DELETED");
        else if (ctx->data[i].state == EMPTY)
            fprintf(out, "EMPTY");
        else
            assert(!"Wrong state!");
        fprintf(out, "\n");
    }
}
