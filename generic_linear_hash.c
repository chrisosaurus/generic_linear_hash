/* The MIT License (MIT)
 *
 * Author: Chris Hall <followingthepath at gmail dot c0m>
 *
 * Copyright (c) 2015 Chris Hall (cjh)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
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

#include <stdio.h> /* puts, printf */
#include <limits.h> /* ULONG_MAX */

#include <stdlib.h> /* calloc, free */
#include <string.h> /* strcmp, strlen */
#include <stddef.h> /* size_t */

#include "generic_linear_hash.h"

/* default number of slots */
#define glh_DEFAULT_SIZE  32

/* factor we grow the number of slots by each resize */
#define glh_SCALING_FACTOR 2

/* default loading factor we resize after in base 10
 * 0 through to 10
 *
 * default is 6 so 60 %
 */
#define glh_DEFAULT_THRESHOLD 6

#define DEBUG 1

/* leaving this in place as we have some internal only helper functions
 * that we only exposed to allow for easy testing and extension
 */
#pragma GCC diagnostic ignored "-Wmissing-prototypes"

/**********************************************
 **********************************************
 **********************************************
 ******** simple helper functions *************
 **********************************************
 **********************************************
 ***********************************************/

/* NOTE these helper functions are not exposed in our header
 * but are not static so as to allow easy unit testing
 * or extension
 */

/* logic for testing if the current entry is eq to the
 * provided hash, and key
 * this is to centralise the once scattered logic
 */
unsigned int glh_entry_eq(const struct glh_table *table, struct glh_entry *cur, unsigned long int hash, const void *key){
    if( ! cur ){
        puts("glh_entry_eq: cur was null");
        return 0;
    }
    if( ! key ){
        puts("glh_entry_eq: key was null");
        return 0;
    }
    if( ! table ){
        puts("glh_entry_eq: table was null");
        return 0;
    }

    if( cur->hash != hash ){
        return 0;
    }

    if( table->equal_func ){
        /* equal func will return 0 for equal
         * and non-zero for not-equal
         */
        if( table->equal_func(cur->key, key) ){
            /* if they are not equal then return 0 */
            return 0;
        }
    }


    return 1;
}

/* initialise an existing glh_entry
 *
 * returns 1 on success
 * returns 0 on failure
 */
unsigned int glh_entry_init(           struct glh_table *table,
                                       struct glh_entry *entry,
                                       unsigned long int hash,
                                       const char *key,
                                       void *data ){

    if( ! entry ){
        puts("glh_entry_init: entry was null");
        return 0;
    }

    if( ! key ){
        puts("glh_entry_init: key was null");
        return 0;
    }

    /* we allow data to be null */

    /* we allow next to be null */

    /* if hash is 0 we issue a warning and recalculate */
    if( hash == 0 ){
        puts("warning glh_entry_init: provided hash was 0, recalculating");
        hash = table->hash_func(key);
    }

    /* setup our simple fields */
    entry->hash    = hash;
    entry->data    = data;
    entry->state   = glh_ENTRY_OCCUPIED;

    /* we duplicate the string */
    entry->key = key;

    /* return success */
    return 1;
}

/* destroy entry
 *
 * will only free *data if `free_data` is 1
 * will NOT free *next
 * will free all other values
 *
 * returns 1 on success
 * returns 0 on failure
 */
unsigned int glh_entry_destroy(struct glh_entry *entry, unsigned int free_data){
    if( ! entry ){
        puts("glh_entry_destroy: entry undef");
        return 0;
    }

    if( free_data && entry->data ){
        free(entry->data);
    }

    return 1;
}


/* find the glh_entry that should be holding this key
 *
 * returns a pointer to it on success
 * return 0 on failure
 */
struct glh_entry * glh_find_entry(const struct glh_table *table, const char *key){
    /* our cur entry */
    struct glh_entry *cur = 0;

    /* hash */
    unsigned long int hash = 0;
    /* position in hash table */
    size_t pos = 0;
    /* iterator through entries */
    size_t i = 0;
    /* cached strlen */


    if( ! table ){
        puts("glh_find_entry: table undef");
        return 0;
    }

    if( ! key ){
        puts("glh_find_entry: key undef");
        return 0;
    }

    /* calculate hash */
    hash = table->hash_func(key);

    /* calculate pos
     * we know table is defined here
     * so glh_pos cannot fail
     */
    pos = glh_pos(hash, table->size);

    /* search pos..size */
    for( i=pos; i < table->size; ++i ){
        cur = &(table->entries[i]);

        /* if this is an empty then we stop */
        if( cur->state == glh_ENTRY_EMPTY ){
            /* failed to find element */
#ifdef DEBUG
            puts("glh_find_entry: failed to find key, encountered empty");
#endif
            return 0;
        }

        /* if this is a dummy then we skip but continue */
        if( cur->state == glh_ENTRY_DUMMY ){
            continue;
        }

        if( ! glh_entry_eq(table, cur, hash, key) ){
            continue;
        }

        return cur;
    }

    /* search 0..pos */
    for( i=0; i < pos; ++i ){
        cur = &(table->entries[i]);

        /* if this is an empty then we stop */
        if( cur->state == glh_ENTRY_EMPTY ){
            /* failed to find element */
#ifdef DEBUG
            puts("glh_find_entry: failed to find key, encountered empty");
#endif
            return 0;
        }

        /* if this is a dummy then we skip but continue */
        if( cur->state == glh_ENTRY_DUMMY ){
            continue;
        }

        if( ! glh_entry_eq(table, cur, hash, key) ){
            continue;
        }

        return cur;
    }

    /* failed to find element */
#ifdef DEBUG
    puts("glh_find_entry: failed to find key");
#endif
    return 0;
}



/**********************************************
 **********************************************
 **********************************************
 ******** generic_linear_hash.h implementation ********
 **********************************************
 **********************************************
 ***********************************************/

/* function to return number of elements
 *
 * returns number on success
 * returns 0 on failure
 */
unsigned int glh_nelems(const struct glh_table *table){
    if( ! table ){
        puts("glh_nelems: table was null");
        return 0;
    }

    return table->n_elems;
}

/* function to calculate load
 * (table->n_elems * 10) / table->size
 *
 * returns loading factor 0 -> 10 on success
 * returns 0 on failure
 */
unsigned int glh_load(const struct glh_table *table){
    if( ! table ){
        puts("glh_load: table was null");
        return 0;
    }

    /* here we multiply by 10 to avoid floating point
     * as we only care about the most significant figure
     */
    return (table->n_elems * 10) / table->size;
}

/* set the load that we resize at
 * load is (table->n_elems * 10) / table->size
 *
 * this sets glh_table->threshold
 * this defaults to glh_DEFAULT_THRESHOLD in generic_linear_hash.c
 * this is set to 6 (meaning 60% full) by default
 *
 * this will accept any value between 1 (10%) to 10 (100%)
 *
 * returns 1 on success
 * returns 0 on failure
 */
unsigned int glh_tune_threshold(struct glh_table *table, unsigned int threshold){
    if( ! table ){
        puts("glh_tune_threshold: table was null");
        return 0;
    }

    if( threshold < 1 || threshold > 10 ){
        puts("glh_tune_threshold: threshold must be between 1 and 9 (inclusive)");
        return 0;
    }

    table->threshold = threshold;
    return 1;
}

/* takes a table and a hash value
 *
 * returns the index into the table for this hash
 * returns 0 on failure (if table is null)
 *
 * note the error value is indistinguishable from the 0th bucket
 * this function can only error if table is null
 * so the caller can distinguish these 2 cases
 */
size_t glh_pos(unsigned long int hash, size_t table_size){

    /* force hash value into a bucket */
    return hash % table_size;
}

/* allocate and initialise a new glh_table
 *
 * will automatically assume a size of 32
 *
 * glh_table will automatically resize when a call to
 * glh_insert detects the load factor is over table->threshold
 *
 * takes a mandatory hashing function used to hash a key
 * takes an optional equality function used to deal with hash
 * collisions where
 *  hash(a) == hash(b) and a != b
 *
 * returns pointer on success
 * returns 0 on failure
 */
struct glh_table * glh_new(
        unsigned long int (*hash_func)(const void *key),
        unsigned int (*equal_func)(const void *a, const void *b)
    ){

    struct glh_table *sht = 0;

    if( ! hash_func ){
        puts("glh_new: hash_func was undef");
        return 0;
    }

    /* alloc */
    sht = calloc(1, sizeof(struct glh_table));
    if( ! sht ){
        puts("glh_new: calloc failed");
        return 0;
    }

    /* init */
    if( ! glh_init(sht, glh_DEFAULT_SIZE, hash_func, equal_func) ){
        puts("glh_new: call to glh_init failed");
        /* make sure to free our allocate glh_table */
        free(sht);
        return 0;
    }

    return sht;
}

/* free an existing glh_table
 * this will free all the sh entries stored
 * this will free all the keys (as they are strdup-ed)
 *
 * this will only free the *table pointer if `free_table` is set to 1
 * this will only free the *data pointers if `free_data` is set to 1
 *
 * returns 1 on success
 * returns 0 on failure
 */
unsigned int glh_destroy(struct glh_table *table, unsigned int free_table, unsigned int free_data){
    /* iterator through table */
    size_t i = 0;

    if( ! table ){
        puts("glh_destroy: table undef");
        return 0;
    }

    /* iterate through `entries` list
     * calling glh_entry_destroy on each
     */
    for( i=0; i < table->size; ++i ){
        if( ! glh_entry_destroy( &(table->entries[i]), free_data ) ){
            puts("glh_destroy: call to glh_entry_destroy failed, continuing...");
        }
    }

    /* free entires table */
    free(table->entries);

    /* finally free table if asked to */
    if( free_table ){
        free(table);
    }

    return 1;
}

/* initialise an already allocated glh_table to size size
 *
 * returns 1 on success
 * returns 0 on failure
 */
unsigned int glh_init(
        struct glh_table *table,
        size_t size,
        unsigned long int (*hash_func)(const void *key),
        unsigned int (*equal_func)(const void *a, const void *b)
    ){

    if( ! table ){
        puts("glh_init: table undef");
        return 0;
    }

    if( size == 0 ){
        puts("glh_init: specified size of 0, impossible");
        return 0;
    }

    if( ! hash_func ){
        puts("glh_init: hash_func undef");
        return 0;
    }

    table->size       = size;
    table->n_elems    = 0;
    table->threshold  = glh_DEFAULT_THRESHOLD;
    table->hash_func  = hash_func;
    table->equal_func = equal_func;

    /* calloc our buckets (pointer to glh_entry) */
    table->entries = calloc(size, sizeof(struct glh_entry));
    if( ! table->entries ){
        puts("glh_init: calloc failed");
        return 0;
    }

    return 1;
}

/* resize an existing table to new_size
 * this will reshuffle all the buckets around
 *
 * you can use this to make a hash larger or smaller
 *
 * returns 1 on success
 * returns 0 on failure
 */
unsigned int glh_resize(struct glh_table *table, size_t new_size){
    /* our new data area */
    struct glh_entry *new_entries = 0;
    /* the current entry we are copying across */
    struct glh_entry *cur = 0;
    /* our iterator through the old hash */
    size_t i = 0;
    /* our iterator through the new data */
    size_t j = 0;
    /* our new position for each element */
    size_t new_pos = 0;

    if( ! table ){
        puts("glh_resize: table was null");
        return 0;
    }

    if( new_size == 0 ){
        puts("glh_resize: asked for new_size of 0, impossible");
        return 0;
    }

    if( new_size <= table->n_elems ){
        puts("glh_resize: asked for new_size smaller than number of existing elements, impossible");
        return 0;
    }

    /* allocate an array of glh_entry */
    new_entries = calloc(new_size, sizeof(struct glh_entry));
    if( ! new_entries ){
        puts("glh_resize: call to calloc failed");
        return 0;
    }

    /* iterate through old data */
    for( i=0; i < table->size; ++i ){
        cur = &(table->entries[i]);

        /* if we are not occupied then skip */
        if( cur->state != glh_ENTRY_OCCUPIED ){
            continue;
        }

        /* our position within new entries */
        new_pos = glh_pos(cur->hash, new_size);

        for( j = new_pos; j < new_size; ++ j){
            /* skip if not empty */
            if( new_entries[j].state != glh_ENTRY_EMPTY ){
                continue;
            }
            goto glh_RESIZE_FOUND;
        }

        for( j = 0; j < new_pos; ++ j){
            /* skip if not empty */
            if( new_entries[j].state != glh_ENTRY_EMPTY ){
                continue;
            }
            goto glh_RESIZE_FOUND;
        }

        puts("glh_resize: failed to find spot for new element!");
        /* make sure to free our new_entries since we don't store them
         * no need to free items in as they are still held in our old elems
         */
        free(new_entries);
        return 0;

glh_RESIZE_FOUND:
        new_entries[j].hash    = cur->hash;
        new_entries[j].key     = cur->key;
        new_entries[j].data    = cur->data;
        new_entries[j].state   = cur->state;
    }

    /* free old data */
    free(table->entries);

    /* swap */
    table->size = new_size;
    table->entries = new_entries;

    return 1;
}

/* check if the supplied key already exists in this hash
 *
 * returns 1 on success (key exists)
 * returns 0 if key doesn't exist or on failure
 */
unsigned int glh_exists(const struct glh_table *table, const char *key){
    struct glh_entry *she = 0;

    if( ! table ){
        puts("glh_exists: table undef");
        return 0;
    }

    if( ! key ){
        puts("glh_exists: key undef");
        return 0;
    }

#ifdef DEBUG
    printf("glh_exist: called with key '%s', dispatching to glh_find_entry\n", key);
#endif

    /* find entry */
    she = glh_find_entry(table, key);
    if( ! she ){
        /* not found */
        return 0;
    }

    /* found */
    return 1;
}

/* insert `data` under `key`
 * this will only success if !glh_exists(table, key)
 *
 * returns 1 on success
 * returns 0 on failure
 */
unsigned int glh_insert(struct glh_table *table, const char *key, void *data){
    /* our new entry */
    struct glh_entry *she = 0;
    /* hash */
    unsigned long int hash = 0;
    /* position in hash table */
    size_t pos = 0;
    /* iterator through table */
    size_t i = 0;
    /* cached strlen */

    if( ! table ){
        puts("glh_insert: table undef");
        return 0;
    }

    if( ! key ){
        puts("glh_insert: key undef");
        return 0;
    }

#ifdef DEBUG
    printf("glh_insert: asked to insert for key '%s'\n", key);
#endif

    /* we allow data to be 0 */

#ifdef DEBUG
    puts("glh_insert: calling glh_exists");
#endif

    /* check for already existing key
     * insert only works if the key is not already present
     */
    if( glh_exists(table, key) ){
        puts("glh_insert: key already exists in table");
        return 0;
    }

    /* determine if we have to resize
     * note we are checking the load before the insert
     */
    if( glh_load(table) >= table->threshold ){
        if( ! glh_resize(table, table->size * glh_SCALING_FACTOR) ){
            puts("glh_insert: call to glh_resize failed");
            return 0;
        }
    }

    /* calculate hash */
    hash = table->hash_func(key);

    /* calculate pos
     * we know table is defined here
     * so glh_pos cannot fail
     */
    pos = glh_pos(hash, table->size);

#ifdef DEBUG
    printf("glh_insert: trying to insert key '%s', hash value '%zd', starting at pos '%zd'\n", key, hash, pos);
#endif

    /* iterate from pos to size */
    for( i=pos; i < table->size; ++i ){
        she = &(table->entries[i]);
        /* if taken keep searching */
        if( she->state == glh_ENTRY_OCCUPIED ){
            continue;
        }

        /* otherwise (empty or dummy) jump to found */
        goto glh_INSERT_FOUND;
    }

    /* iterate from 0 to pos */
    for( i=0; i < pos; ++i ){
        she = &(table->entries[i]);
        /* if taken keep searching */
        if( she->state == glh_ENTRY_OCCUPIED ){
            continue;
        }

        /* otherwise (empty or dummy) jump to found */
        goto glh_INSERT_FOUND;
    }

    /* no slot found */
    puts("glh_insert: unable to find insertion slot");
    return 0;

glh_INSERT_FOUND:
    /* she is already set! */

#ifdef DEBUG
    printf("glh_insert: inserting insert key '%s', hash value '%zd', starting at pos '%zd', into '%zd'\n", key, hash, pos, i);
#endif

    /* construct our new glh_entry
     * glh_entry_new(unsigned long int hash,
     *              char *key,
     *              void *data,
     *              struct glh_entry *next){
     *
     * only key needs to be defined
     *
     */
    /*                  (table, entry, hash, key,data) */
    if( ! glh_entry_init(table, she,   hash, key, data) ){
        puts("glh_insert: call to glh_entry_init failed");
        return 0;
    }

    /* increment number of elements */
    ++table->n_elems;

    /* return success */
    return 1;
}

/* set `data` under `key`
 * this will only succeed if glh_exists(table, key)
 *
 * returns old data on success
 * returns 0 on failure
 */
void * glh_set(struct glh_table *table, const char *key, void *data){
    struct glh_entry *she = 0;
    void * old_data = 0;

    if( ! table ){
        puts("glh_set: table undef");
        return 0;
    }

    if( ! key ){
        puts("glh_set: key undef");
        return 0;
    }

    /* allow data to be null */

    /* find entry */
    she = glh_find_entry(table, key);
    if( ! she ){
        /* not found */
        return 0;
    }

    /* save old data */
    old_data = she->data;

    /* overwrite */
    she->data = data;

    /* return old data */
    return old_data;
}

/* get `data` stored under `key`
 *
 * returns data on success
 * returns 0 on failure
 */
void * glh_get(const struct glh_table *table, const char *key){
    struct glh_entry *she = 0;

    if( ! table ){
        puts("glh_get: table undef");
        return 0;
    }

    if( ! key ){
        puts("glh_get: key undef");
        return 0;
    }

    /* find entry */
    she = glh_find_entry(table, key);
    if( ! she ){
        /* not found */
        return 0;
    }

    /* found */
    return she->data;
}

/* delete entry stored under `key`
 *
 * returns data on success
 * returns 0 on failure
 */
void * glh_delete(struct glh_table *table, const char *key){
    /* our cur entry */
    struct glh_entry *cur = 0;
    /* hash */
    unsigned long int hash = 0;
    /* position in hash table */
    size_t pos = 0;
    /* iterator through has table */
    size_t i =0;
    /* cached strlen */

    /* our old data */
    void *old_data = 0;

    if( ! table ){
        puts("glh_delete: table undef");
        return 0;
    }

    if( ! key ){
        puts("glh_delete: key undef");
        return 0;
    }

    /* calculate hash */
    hash = table->hash_func(key);

    /* calculate pos
     * we know table is defined here
     * so glh_pos cannot fail
     */
    pos = glh_pos(hash, table->size);


    /* starting at pos search for element
     * searches pos .. size
     */
    for( i = pos; i < table->size ; ++i ){
        cur = &(table->entries[i]);

        /* if this is an empty then we stop */
        if( cur->state == glh_ENTRY_EMPTY ){
            /* failed to find element */
#ifdef DEBUG
            puts("glh_delete: failed to find key, encountered empty");
#endif
            return 0;
        }

        /* if this is a dummy then we skip but continue */
        if( cur->state == glh_ENTRY_DUMMY ){
            continue;
        }

        if( ! glh_entry_eq(table, cur, hash, key) ){
            continue;
        }

        goto glh_DELETE_FOUND;
    }

    /* if we are here then we hit the end,
     * searches 0 .. pos
     */
    for( i = 0; i < pos; ++i ){
        cur = &(table->entries[i]);

        /* if this is an empty then we stop */
        if( cur->state == glh_ENTRY_EMPTY ){
            /* failed to find element */
#ifdef DEBUG
            puts("glh_delete: failed to find key, encountered empty");
#endif
            return 0;
        }

        /* if this is a dummy then we skip but continue */
        if( cur->state == glh_ENTRY_DUMMY ){
            continue;
        }

        if( ! glh_entry_eq(table, cur, hash, key) ){
            continue;
        }

        goto glh_DELETE_FOUND;
    }

    /* failed to find element */
    puts("glh_delete: failed to find key, both loops terminated");
    return 0;

glh_DELETE_FOUND:
        /* cur is already set! */

        /* save old data pointer */
        old_data = cur->data;

        /* clear out */
        cur->data = 0;
        cur->key = 0;
        cur->hash = 0;
        cur->state = glh_ENTRY_DUMMY;

        /* decrement number of elements */
        --table->n_elems;

        /* return old data */
        return old_data;
}


