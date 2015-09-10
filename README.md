# Generic linear hash [![Build Status](https://travis-ci.org/mkfifo/generic_linear_hash.svg)](https://travis-ci.org/mkfifo/generic_linear_hash) [![Coverage Status](https://coveralls.io/repos/mkfifo/generic_linear_hash/badge.svg?service=github)](https://coveralls.io/github/mkfifo/generic_linear_hash)

An implementation of a linear probing hash table written in pure C99 with no external dependencies, allows for generic keys

Generic linear hash is licensed under the MIT license, see LICENSE for more details


Example usage:
--------------

    #include <string.h> /* strlen */

    #include "generic_linear_hash.h"

    #pragma GCC diagnostic ignored "-Wunused-but-set-variable"

    unsigned long int hash_func(const void *key_void);

    int main(void){
        /* create a hash
         * the hash will automatically manage
         * it's size
         */
        struct glh_table *t = glh_new(hash_func, 0);

        /* some data to store */
        int data_1 = 1;
        int data_2 = 2;

        int *data;

        /* insert new data */
        glh_insert(t, "hello", &data_1);
        glh_insert(t, "world", &data_2);

        /* fetch */
        data = glh_get(t, "hello");

        /* delete existing data */
        glh_delete(t, "world");

        /* mutate existing data */
        glh_set(t, "hello", &data_2);

        /* check a key exists */
        if( glh_exists(t, "hello") ){
        }

        /* tidy up
         * free table
         * but do not free stored data
         * destroy(table, free_table, free_data) */
        glh_destroy(t,     1,          0);

        return 0;
    }

    unsigned long int hash_func(const void *key_void){
        unsigned int key_len = 0;
        /* our hash value */
        unsigned long int hash = 0;
        /* our iterator through the key */
        size_t i = 0;
        const char * key = 0;

        key = key_void;

        /* old lh_hash */

        if( ! key ){
            return 0;
        }

        key_len = strlen(key);

        /* hashing time */
        for( i=0; i < key_len; ++i ){
            /* hash this character
             * http://www.cse.yorku.ca/~oz/hash.html
             * djb2
             */
            hash = ((hash << 5) + hash) + key[i];
        }
        return hash;
    }

