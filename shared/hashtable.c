/**
 * @file shared/hashtable.c
 * @brief an hashtable implementation
 */

#include <string.h>
#include <stdlib.h>

#include "cpp.h"
#include "nearest_power.h"
#include "hashtable.h"
#include "utils.h"

#define HASHTABLE_MIN_SIZE 8

ht_hash_t value_hash(ht_key_t k)
{
    return (ht_hash_t) k;
}

bool value_equal(ht_key_t k1, ht_key_t k2)
{
    return k1 == k2;
}

bool ascii_equal_cs(ht_key_t k1, ht_key_t k2)
{
    const char *string1 = (const char *) k1;
    const char *string2 = (const char *) k2;

    if (NULL == string1 || NULL == string2) {
        return string1 == string2;
    }

    return 0 == strcmp(string1, string2);
}

ht_hash_t ascii_hash_cs(ht_key_t k)
{
    ht_hash_t h = 5381;
    const char *str = (const char *) k;

    while (0 != *str) {
        h += (h << 5);
        h ^= (unsigned long) *str++;
    }

    return h;
}

bool ascii_equal_ci(ht_key_t k1, ht_key_t k2)
{
    const char *string1 = (const char *) k1;
    const char *string2 = (const char *) k2;

    if (NULL == string1 || NULL == string2) {
        return string1 == string2;
    }

    return 0 == ascii_strcasecmp(string1, string2);
}

ht_hash_t ascii_hash_ci(ht_key_t k)
{
    ht_hash_t h = 5381;
    const char *str = (const char *) k;

    while (0 != *str) {
        h += (h << 5);
        h ^= (unsigned long) ascii_toupper((unsigned char) *str++);
    }

    return h;
}

static inline void hashtable_rehash(HashTable *ht)
{
    HashNode *n;
    uint32_t index;

    if (UNEXPECTED(ht->count < 1)) {
        return;
    }
    memset(ht->nodes, 0, ht->capacity * sizeof(*ht->nodes));
    n = ht->gHead;
    while (NULL != n) {
        index = n->hash & ht->mask;
        n->nNext = ht->nodes[index];
        n->nPrev = NULL;
        if (NULL != n->nNext) {
            n->nNext->nPrev = n;
        }
        ht->nodes[index] = n;
        n = n->gNext;
    }
}

static inline void hashtable_maybe_resize(HashTable *ht)
{
    if (UNEXPECTED(ht->count < ht->capacity)) {
        return;
    }
    if (EXPECTED(ht->capacity << 1) > 0) {
        ht->nodes = mem_renew(ht->nodes, *ht->nodes, ht->capacity << 1);
        ht->capacity <<= 1;
        ht->mask = ht->capacity - 1;
        hashtable_rehash(ht);
    }
}

/**
 * Initialize a hashtable
 *
 * @param ht the hashtable to set
 * @param capacity the initial capacity of the hashtable
 * @param hf callback to hash keys
 * @param ef callback to determine if two keys are equal, if NULL, it will be set to value_equal
 * @param key_duper the keys duper (NULL to use them as is/without copying them)
 * @param key_dtor the key destructor (NULL to not destroy them automatically)
 * @param value_dtor the value destructor (NULL to not destroy them automatically)
 */
void hashtable_init(
    HashTable *ht,
    size_t capacity,
    HashFunc hf,
    EqualFunc ef,
    DupFunc key_duper,
    DtorFunc key_dtor,
    DtorFunc value_dtor
) {
    ht->count = 0;
    ht->gHead = NULL;
    ht->gTail = NULL;
    ht->capacity = nearest_power(capacity, HASHTABLE_MIN_SIZE);
    ht->mask = ht->capacity - 1;
    ht->hf = hf;
    if (NULL == ef) {
        ht->ef = value_equal;
    } else {
        ht->ef = ef;
    }
    ht->key_duper = key_duper;
    ht->key_dtor = key_dtor;
    ht->value_dtor = value_dtor;
    ht->nodes = mem_new_n(*ht->nodes, ht->capacity);
    memset(ht->nodes, 0, ht->capacity * sizeof(*ht->nodes));
}

/**
 * Helper to initialize a hashtable for binary/case sensitive strings as keys
 *
 * @param ht the hashtable to set
 * @param key_duper the keys duper (NULL to use them as is/without copying them)
 * @param key_dtor the key destructor (NULL to not destroy them automatically)
 * @param value_dtor the value destructor (NULL to not destroy them automatically)
 */
void hashtable_ascii_cs_init(HashTable *ht, DupFunc key_duper, DtorFunc key_dtor, DtorFunc value_dtor)
{
    hashtable_init(ht, HASHTABLE_MIN_SIZE, ascii_hash_cs, ascii_equal_cs, key_duper, key_dtor, value_dtor);
}

/**
 * Helper to initialize a hashtable for (ASCII) case insensitive strings as keys
 *
 * @param ht the hashtable to set
 * @param key_duper the keys duper (NULL to use them as is/without copying them)
 * @param key_dtor the key destructor (NULL to not destroy them automatically)
 * @param value_dtor the value destructor (NULL to not destroy them automatically)
 */
void hashtable_ascii_ci_init(HashTable *ht, DupFunc key_duper, DtorFunc key_dtor, DtorFunc value_dtor)
{
    hashtable_init(ht, HASHTABLE_MIN_SIZE, ascii_hash_ci, ascii_equal_ci, key_duper, key_dtor, value_dtor);
}

/**
 * Compute the hash for the given key
 *
 * @param ht the hashtable
 * @param key the key to hash
 *
 * @return the computed hash for key
 */
ht_hash_t _hashtable_hash(HashTable *ht, ht_key_t key)
{
    assert(NULL != ht);

    return NULL == ht->hf ? key : ht->hf(key);
}

/**
 * Get items count
 *
 * @param ht the hashtable
 *
 * @return the number of elements inside ht
 */
size_t hashtable_size(HashTable *ht)
{
    assert(NULL != ht);

    return ht->count;
}

/**
 * Real work to insert a new item or to overwrite an existing one
 *
 * @param ht the hashtable
 * @param flags a mask of the following options:
 *   - HT_PUT_ON_DUP_KEY_PRESERVE: if the key already exists, do not overwrite its current value
 * @param h the hash of the key
 * @param key the key of the item to put
 * @param value its associated value
 * @param oldvalue if this pointer is not NULL, it will receive the previous value associated to the key
 *
 * @return false if the hashtable is unchanged (no insertion or modification)
 */
static bool hashtable_put_real(HashTable *ht, uint32_t flags, ht_hash_t h, ht_key_t key, void *value, void **oldvalue)
{
    HashNode *n;
    uint32_t index;

    assert(NULL != ht);

    index = h & ht->mask;
    n = ht->nodes[index];
    while (NULL != n) {
        if (n->hash == h && ht->ef(key, n->key)) {
            if (NULL != oldvalue) {
                *oldvalue = n->data;
            }
            if (!HAS_FLAG(flags, HT_PUT_ON_DUP_KEY_PRESERVE)) {
                if (NULL != ht->value_dtor/* && !HAS_FLAG(flags, HT_PUT_ON_DUP_KEY_NO_DTOR)*/) {
                    ht->value_dtor(n->data);
                }
                n->data = value;
                return true;
            }
            return false;
        }
        n = n->nNext;
    }
    n = mem_new(*n);
    if (NULL == ht->key_duper) {
        n->key = key;
    } else {
        n->key = (ht_key_t) ht->key_duper((void *) key);
    }
    n->hash = h;
    n->data = value;
    // Bucket: prepend
    n->nNext = ht->nodes[index];
    n->nPrev = NULL;
    if (NULL != n->nNext) {
        n->nNext->nPrev = n;
    }
    // Global
    n->gPrev = ht->gTail;
    ht->gTail = n;
    n->gNext = NULL;
    if (NULL != n->gPrev) {
        n->gPrev->gNext = n;
    }
    if (NULL == ht->gHead) {
        ht->gHead = n;
    }
    ht->nodes[index] = n;
    ++ht->count;
    hashtable_maybe_resize(ht);

    return true;
}

/**
 * Put a key/value when key's hash is already known
 *
 * @param ht the hashtable
 * @param flags a mask of the following options:
 *   - HT_PUT_ON_DUP_KEY_PRESERVE: if the key already exists, do not overwrite its current value
 * @param h the hash of the key
 * @param key the key of the item to put
 * @param value its associated value
 * @param oldvalue if this pointer is not NULL, it will receive the previous value associated to the key
 *
 * @return false if the hashtable is unchanged (no insertion or modification)
 */
bool _hashtable_quick_put(HashTable *ht, uint32_t flags, ht_hash_t h, ht_key_t key, void *value, void **oldvalue)
{
    return hashtable_put_real(ht, flags, h, key, value, oldvalue);
}

/**
 * Put a key/value (normal case)
 *
 * @param ht the hashtable
 * @param flags a mask of the following options:
 *   - HT_PUT_ON_DUP_KEY_PRESERVE: if the key already exists, do not overwrite its current value
 * @param key the key of the item to put
 * @param value its associated value
 * @param oldvalue if this pointer is not NULL, it will receive the previous value associated to the key
 *
 * @return false if the hashtable is unchanged (no insertion or modification)
 */
bool _hashtable_put(HashTable *ht, uint32_t flags, ht_key_t key, void *value, void **oldvalue)
{
    return hashtable_put_real(ht, flags, NULL == ht->hf ? key : ht->hf(key), key, value, oldvalue);
}

/**
 * Put a key/value when key is also the hash
 *
 * @param ht the hashtable
 * @param flags a mask of the following options:
 *   - HT_PUT_ON_DUP_KEY_PRESERVE: if the key already exists, do not overwrite its current value
 * @param h the key and hash
 * @param value its associated value
 * @param oldvalue if this pointer is not NULL, it will receive the previous value associated to the key
 *
 * @return false if the hashtable is unchanged (no insertion or modification)
 */
bool _hashtable_direct_put(HashTable *ht, uint32_t flags, ht_hash_t h, void *value, void **oldvalue)
{
    return hashtable_put_real(ht, flags, h, (ht_key_t) h, value, oldvalue);
}

/**
 * Test if a hashtable contains a key from the key and its hash
 *
 * @param ht the hashtable
 * @param h the hash of the key
 * @param key the key to lookup
 *
 * @return true if the key exists in ht
 */
bool _hashtable_quick_contains(HashTable *ht, ht_hash_t h, ht_key_t key)
{
    HashNode *n;
    uint32_t index;

    assert(NULL != ht);

    index = h & ht->mask;
    n = ht->nodes[index];
    while (NULL != n) {
        if (n->hash == h && ht->ef(key, n->key)) {
            return true;
        }
        n = n->nNext;
    }

    return false;
}

/**
 * Test if a hashtable contains a key
 *
 * @param ht the hashtable
 * @param key the key to lookup
 *
 * @return true if the key exists in ht
 */
bool _hashtable_contains(HashTable *ht, ht_key_t key)
{
    return _hashtable_quick_contains(ht, NULL == ht->hf ? key : ht->hf(key), key);
}

/**
 * Test if a hashtable contains a key which is also its hash (eg a number - with a type compatible with ht_key_t)
 *
 * @param ht the hashtable
 * @param h the key and hash to lookup
 *
 * @return true if the key exists in ht
 */
bool _hashtable_direct_contains(HashTable *ht, ht_hash_t h)
{
    return _hashtable_quick_contains(ht, h, (ht_key_t) h);
}

/**
 * Get the value associated to a key from the key and its hash
 *
 * @param ht the hashtable
 * @param h the hash of the key (if you don't know it or don't want to compute it yourselves,
 * use hashtable_get instead)
 * @param key the key to look for
 * @param value a pointer to receive the current value associated to this key
 *
 * @return false if the key does not exist
 */
bool _hashtable_quick_get(HashTable *ht, ht_hash_t h, ht_key_t key, void **value)
{
    HashNode *n;
    uint32_t index;

    assert(NULL != ht);
    assert(NULL != value);

    index = h & ht->mask;
    n = ht->nodes[index];
    while (NULL != n) {
        if (n->hash == h && ht->ef(key, n->key)) {
            *value = n->data;
            return true;
        }
        n = n->nNext;
    }

    return false;
}

/**
 * Get the value associated to a key
 *
 * @param ht the hashtable
 * @param key the key to look for
 * @param value a pointer to receive the current value associated to this key
 *
 * @return false if the key does not exist
 */
bool _hashtable_get(HashTable *ht, ht_key_t key, void **value)
{
    return _hashtable_quick_get(ht, NULL == ht->hf ? key : ht->hf(key), key, value);
}

/**
 * Get the value associated to a key which is also the hash
 *
 * @param ht the hashtable
 * @param h the key and hash to look for
 * @param value a pointer to receive the current value associated to this key
 *
 * @return false if the key does not exist
 */
bool _hashtable_direct_get(HashTable *ht, ht_hash_t h, void **value)
{
    return _hashtable_quick_get(ht, h, h, value);
}

/**
 * Remove a node while traversing a hashtable (low level API)
 *
 * \code
 *   HashNode *n;
 *
 *   n = g->terminals.gHead;
 *   while (NULL != n) {
 *       if (some condition) {
 *           n = hashtable_delete_node(ht, n); // delete current node and move to next one
 *       } else {
 *           n = n->gNext; // normal case
 *       }
 *   }
 * \endcode
 *
 * @param ht the hashtable
 * @param n the node to remove
 *
 * @return the next node (may be NULL)
 */
HashNode *hashtable_delete_node(HashTable *ht, HashNode *n)
{
    HashNode *ret;

    if (NULL != n->nPrev) {
        n->nPrev->nNext = n->nNext;
    } else {
        uint32_t index;

        index = n->hash & ht->mask;
        ht->nodes[index] = n->nNext;
    }
    if (NULL != n->nNext) {
        n->nNext->nPrev = n->nPrev;
    }
    if (NULL != n->gPrev) {
        n->gPrev->gNext = n->gNext;
    } else {
        ht->gHead = n->gNext;
    }
    if (NULL != n->gNext) {
        n->gNext->gPrev = n->gPrev;
    } else {
        ht->gTail = n->gPrev;
    }
    --ht->count;
    if (NULL != ht->value_dtor) {
        ht->value_dtor(n->data);
    }
    if (NULL != ht->key_dtor) {
        ht->key_dtor((void *) n->key);
    }
    ret = n->gNext;
    free(n);

    return ret;
}

/**
 * Real work for item deletion
 *
 * @param ht the hashtable
 * @param h the hash of the item to remove
 * @param key the key of the item to remove
 * @param call_dtor false to not call value destructor
 *
 * @return true if an item has been deleted
 */
static bool hashtable_delete_real(HashTable *ht, ht_hash_t h, ht_key_t key, bool call_dtor)
{
    HashNode *n;
    uint32_t index;

    assert(NULL != ht);

    index = h & ht->mask;
    n = ht->nodes[index];
    while (NULL != n) {
        if (n->hash == h && ht->ef(key, n->key)) {
            if (n == ht->nodes[index]) { // or NULL == n->nPrev
                ht->nodes[index] = n->nNext;
            } else {
                n->nPrev->nNext = n->nNext;
            }
            if (NULL != n->nNext) {
                n->nNext->nPrev = n->nPrev;
            }
            if (NULL != n->gPrev) {
                n->gPrev->gNext = n->gNext;
            } else {
                ht->gHead = n->gNext;
            }
            if (NULL != n->gNext) {
                n->gNext->gPrev = n->gPrev;
            } else {
                ht->gTail = n->gPrev;
            }
            if (call_dtor && NULL != ht->value_dtor) {
                ht->value_dtor(n->data);
            }
            if (call_dtor && NULL != ht->key_dtor) {
                ht->key_dtor((void *) n->key);
            }
            free(n);
            --ht->count;
            return true;
        }
        n = n->nNext;
    }

    return false;
}

/**
 * Delete an element from its key and hash
 *
 * Its purpose is to avoid an unnecessary internal call to the hash function if the hash
 * of the element is already known.
 *
 * @param ht the hashtable
 * @param h the hash of the item to remove
 * @param key the key of the item to remove
 * @param call_dtor false to not call value destructor
 *
 * @return true if an item has been deleted
 */
bool _hashtable_quick_delete(HashTable *ht, ht_hash_t h, ht_key_t key, bool call_dtor)
{
    return hashtable_delete_real(ht, h, key, call_dtor);
}

/**
 * Delete an element from its key
 *
 * @param ht the hashtable
 * @param key the key of the item to remove
 * @param call_dtor false to not call value destructor
 *
 * @return true if an item has been deleted
 */
bool _hashtable_delete(HashTable *ht, ht_key_t key, bool call_dtor)
{
    return hashtable_delete_real(ht, NULL == ht->hf ? key : ht->hf(key), key, call_dtor);
}

/**
 * Delete an element from its key which is also its hash
 *
 * @param ht the hashtable
 * @param h the key and hash of the item to remove
 * @param call_dtor false to not call value destructor
 *
 * @return true if an item has been deleted
 */
bool _hashtable_direct_delete(HashTable *ht, ht_hash_t h, bool call_dtor)
{
    return hashtable_delete_real(ht, h, h, call_dtor);
}

/**
 * Common helper to clear/free memory internally used by a hashtable
 *
 * @param ht the hashtable to destroy or clear
 */
static void hashtable_clear_real(HashTable *ht)
{
    HashNode *n, *tmp;

    n = ht->gHead;
    ht->count = 0;
    ht->gHead = NULL;
    ht->gTail = NULL;
    while (NULL != n) {
        tmp = n;
        n = n->gNext;
        if (NULL != ht->value_dtor) {
            ht->value_dtor(tmp->data);
        }
        if (NULL != ht->key_dtor) {
            ht->key_dtor((void *) tmp->key);
        }
        free(tmp);
    }
    memset(ht->nodes, 0, ht->capacity * sizeof(*ht->nodes));
}

/**
 * Clear a hashtable to be reused
 *
 * @param ht the hashtable to clear
 */
void hashtable_clear(HashTable *ht)
{
    assert(NULL != ht);

    hashtable_clear_real(ht);
}

/**
 * Destroy a hashtable
 *
 * @note if the hashtable was allocated on heap, you have to free(ht) after
 *
 * @param ht the hashtable to destroy
 */
void hashtable_destroy(HashTable *ht)
{
    assert(NULL != ht);

    hashtable_clear_real(ht);
    free(ht->nodes);
    //free(ht); // caller have to do this
}

/**
 * Get the value of the first inserted (not modified) element
 *
 * @param ht the hashtable
 *
 * @return the value of the first inserted element (NULL if the hashtable is empty)
 */
void *hashtable_first(HashTable *ht)
{
    if (NULL == ht->gHead) {
        return NULL;
    } else {
        return ht->gHead->data;
    }
}

/**
 * Get the value of the last inserted (not modified) element
 *
 * @param ht the hashtable
 *
 * @return the value of the last inserted element (NULL if the hashtable is empty)
 */
void *hashtable_last(HashTable *ht)
{
    if (NULL == ht->gTail) {
        return NULL;
    } else {
        return ht->gTail->data;
    }
}

/**
 * Copy a hashtable
 *
 * @param dst the HashTable which receives the copy, NULL to create/allocate a new one
 * @param src the HashTable to copy
 * @param key_duper the callback used for the operation to copy the keys. Use NULL to not
 * copy them and (void *) 1 to keep src callback.
 * @param value_duper the callback used to copy values. Use NULL to share the pointers on
 * data
 *
 * @return the copy
 */
HashTable *hashtable_copy(HashTable *dst, HashTable *src, DupFunc key_duper, DupFunc value_duper)
{
    HashNode *n;
    HashTable *ret;
    DupFunc orig_key_duper;

    orig_key_duper = src->key_duper;
    if (NULL == dst) {
        ret = malloc(sizeof(*ret));
        hashtable_init(ret, src->count, src->hf, src->ef, ((void *) 1) == key_duper ? src->key_duper : key_duper, src->key_dtor, src->value_dtor);
    } else {
        ret = dst;
        hashtable_clear(dst);
        dst->hf = src->hf;
        dst->ef = src->ef;
        dst->key_duper = ((void *) 1) == key_duper ? src->key_duper : key_duper;
        dst->key_dtor = src->key_dtor;
        dst->value_dtor = src->value_dtor;
    }
    for (n = src->gHead; NULL != n; n = n->gNext) {
        hashtable_quick_put(ret, 0, n->hash, n->key, NULL == value_duper ? n->data : value_duper(n->data), NULL);
    }
    dst->key_duper = orig_key_duper;

    return ret;
}

/**
 * Merge two hashtables (dst = set1 | set2)
 *
 * Notes:
 * - if set1 and set2 both have a same key, set1 has precedence: the key and value defined
 * by set2 are ignored.
 * - dst can be set1 or set2 (dst |= set2 if set1 == dst)
 *
 * @param dst the HashTable which receives the results, NULL to create/allocate a new one
 * @param set1 one of the two hashtables to merge
 * @param set2 the other hashtable to merge
 * @param key_duper the callback used for the operation to copy the keys. Use NULL to not
 * copy them and (void *) 1 to keep src callback.
 * @param value_duper the callback used to copy values. Use NULL to share the pointers on
 * data
 *
 * @return the resulting hashtable
 */
HashTable *hashtable_union(HashTable *dst, HashTable *set1, HashTable *set2, DupFunc key_duper, DupFunc value_duper)
{
    HashTable *ret;

    ret = NULL;
    if (set1->hf == set2->hf && set1->ef == set2->ef) {
        HashNode *n;
        DupFunc orig_key_duper;

        if (NULL == dst || (dst != set1 && dst != set2)) {
            ret = hashtable_copy(dst, set1, key_duper, value_duper);
        } else {
            ret = dst == set1 ? set1 : set2;
        }
        orig_key_duper = dst->key_duper;
        dst->key_duper = ((void *) 1) == key_duper ? dst->key_duper : key_duper;
        for (n = set2->gHead; NULL != n; n = n->gNext) {
            if (!hashtable_quick_put(dst, HT_PUT_ON_DUP_KEY_PRESERVE, n->hash, n->key, n->data, NULL) && NULL != key_duper) {
                hashtable_quick_put(dst, HT_PUT_ON_DUP_KEY_PRESERVE, n->hash, n->key, value_duper(n->data), NULL);
            }
        }
        dst->key_duper = orig_key_duper;
    }

    return ret;
}
