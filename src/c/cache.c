/*
 * Waxeye Parser Generator
 * www.waxeye.org
 * Copyright (C) 2008 Orlando D. A. R. Hill
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <assert.h>
#include <stdlib.h>

#define CACHE_C_
#include "cache.h"


void cache_value_init(struct cache_value_t *v, struct ast_t *result, size_t pos, size_t line, size_t column, bool last_cr) {
    v->result = result;
    v->pos = pos;
    v->line = line;
    v->column = column;
    v->last_cr = last_cr;
}


struct cache_value_t* cache_value_new(struct ast_t *result, size_t pos, size_t line, size_t column, bool last_cr) {
    struct cache_value_t *v = malloc(sizeof(struct cache_value_t));
    assert(v != NULL);
    cache_value_init(v, result, pos, line, column, last_cr);
    return v;
}


/*
 * Frees the cache_value, deallocates the memory at the given pointer.
 * v - The cache_value to delete.
 */
void cache_value_delete(struct cache_value_t *v) {
    free(v);
}


size_t cache_hash_key(struct cache_key_t *key) {
    return 17 + (37 * key->index) + (37 * key->pos);
}


/*
 * Gets the position in the cache that the key maps to.
 * Doesn't consider hash collisions.
 */
size_t cache_start_pos(struct ht_t *v, struct cache_key_t *key) {
    return abs(cache_hash_key(key) % v->capacity);
}


/*
 * Gets the position that the key maps to after hash collisions are resolved.
 */
size_t cache_final_pos(struct ht_t *v, struct cache_key_t *key) {
    size_t offset = 0;
    size_t pos = cache_start_pos(v, key);
    struct ht_pair_t *pair = v->pairs[pos];

    while (true) {
        // If the position is empty
        if (pair == NULL) {
            return pos;
        }

        // If we found our key
        if (((struct cache_key_t*) pair->key.as_p)->index == key->index &&
            ((struct cache_key_t*) pair->key.as_p)->pos == key->pos) {
            return pos;
        }

        offset++;
        pos = abs((pos + offset * offset) % v->capacity);
        pair = v->pairs[pos];
    }
}


/*
 * Gets the value that is mapped to the given key. If there is no mapping then
 * NULL is returned.
 */
struct cache_value_t* cache_get(struct ht_t *v, struct cache_key_t *key) {
    struct ht_pair_t *pair = v->pairs[cache_final_pos(v, key)];

    if (pair == NULL) {
        return NULL;
    }

    return pair->value;
}


/*
 * Maps the given value against the given key. The value is allowed to be NULL.
 * If a value has previously been mapped against an equal key, the old mapping
 * will be overwritten. The given key location is not stored so it is same to
 * give a key allocated on the stack.
 */
void cache_put(struct ht_t *v, struct cache_key_t *key, struct cache_value_t *value) {
    assert(v != NULL);
    assert(key != NULL);

    const float LOAD_FACTOR = 0.75;

    // resize and rehash if needed
    if (v->size >= v->capacity * LOAD_FACTOR) {

        size_t old_capacity = v->capacity;
        struct ht_pair_t **old_pairs = v->pairs;

        v->capacity = v->capacity * 2;
        assert(v->capacity > v->size);

        v->pairs = calloc(v->capacity, sizeof(void*));
        assert(v->pairs != NULL);

        // rehash the pairs
        size_t i;
        for (i = 0; i < old_capacity; i++) {
            struct ht_pair_t *pair = old_pairs[i];

            if (pair != NULL) {
                v->pairs[cache_final_pos(v, pair->key.as_p)] = pair;
            }
        }

        // free old storage
        free(old_pairs);
    }

    size_t pos = cache_final_pos(v, key);
    struct ht_pair_t *pair = v->pairs[pos];

    // if a value didn't already exist with that key
    if (pair == NULL) {
        // create a new pair
        pair = malloc(sizeof(struct ht_pair_t));
        assert(pair != NULL);

        v->pairs[pos] = pair;
        v->size++;

        // create a new key for the pair
        struct cache_key_t *k = malloc(sizeof(struct cache_key_t));
        assert(k != NULL);

        k->index = key->index;
        k->pos = key->pos;
        pair->key.as_p = k;
    }

    // store the value
    pair->value = value;
}
