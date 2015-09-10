/*  gcc generic_linear_hash.c test_generic_linear_hash.c -Wall -Wextra -Werror -o test_lh
 * ./test_lh
 */
#include <assert.h> /* assert */
#include <stdio.h> /* puts */
#include <stdlib.h> /* calloc */
#include <string.h> /* strcmp */

#include "generic_linear_hash.h"

/* headers for internal functions within generic_linear_hash.c
 * that are not exposed via the header
 * these would be static but we want to be able to test them
 */
unsigned int glh_entry_eq(const struct glh_table *table, struct glh_entry *cur, unsigned long int hash, const void *key);
unsigned int glh_entry_init(           struct glh_table *table,
                                       struct glh_entry *entry,
                                       unsigned long int hash,
                                       const char *key,
                                       void *data );

unsigned int glh_entry_destroy(struct glh_entry *entry, unsigned int free_data);
struct glh_entry * glh_find_entry(struct glh_table *table, char *key);

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
        puts("hash_func: key was null");
        return 0;
    }

    key_len = strlen(key);

    /* hashing time */
    for( i=0; i < key_len; ++i ){

        /* we do not have to worry about overflow doing silly things:
         *
         * C99 section 6.2.5.9 page 34:
         * A computation involving unsigned operands can never overﬂow,
         * because a result that cannot be represented by the resulting
         * unsigned integer type is reduced modulo the number that is one
         * greater than the largest value that can be represented by the
         * resulting type.
         */

        /* hash this character
         * http://www.cse.yorku.ca/~oz/hash.html
         * djb2
         */
        hash = ((hash << 5) + hash) + key[i];
    }

    return hash;
}

unsigned int equal_func(const void *a, const void *b){
    if( ! a ){
        return 0;
    }
    if( ! b ){
        return 0;
    }
    return strcmp(a, b);
}


void new_insert_get_destroy(void){
    /* our simple hash table */
    struct glh_table *table = 0;

    /* some keys */
    char *key_1 = "bbbbb";
    char *key_2 = "aaaaa";
    char *key_3 = "ccccc";

    /* some data */
    int data_1 = 1;
    int data_2 = 2;
    int data_3 = 3;

    /* temporary data pointer used for testing get */
    int *data = 0;


    puts("\ntesting basic functionality");

    puts("testing new");
    table = glh_new(hash_func, equal_func);
    assert(table);
    assert( 32 == table->size );
    assert( 0 == glh_nelems(table) );
    assert( 0 == glh_load(table) );


    puts("testing insert and get");
    puts("one insert");
    assert( glh_insert(table, key_1, &data_1) );
    assert( 1 == glh_nelems(table));
    assert( 0 == glh_get(table, key_2) );
    assert( 0 == glh_get(table, key_3) );

    puts("one get");
    data = glh_get(table, key_1);
    assert(data);
    assert( data_1 == *data );


    puts("two insert");
    assert( glh_insert(table, key_2, &data_2) );
    assert( 2 == glh_nelems(table) );
    assert( 0 == glh_get(table, key_3) );

    puts("two get");
    data = glh_get(table, key_2);
    assert(data);
    assert( data_2 == *data );


    puts("three insert");
    assert( glh_insert(table, key_3, &data_3) );
    assert( 3 == glh_nelems(table) );

    puts("three get");
    data = glh_get(table, key_3);
    assert(data);
    assert( data_3 == *data );


    assert( glh_destroy(table, 1, 0) );
    puts("success!");
}

void set(void){
    /* our simple hash table */
    struct glh_table *table = 0;

    /* some keys */
    char *key_1 = "rhubarb";
    char *key_2 = "carrot";
    char *key_3 = "potato";

    /* some data */
    int data_1 = 1;
    int data_2 = 2;
    int data_3 = 3;

    /* some data we override with */
    int new_data_1 = 14;
    int new_data_2 = 15;
    int new_data_3 = 16;


    /* temporary data pointer used for testing get */
    int *data = 0;

    puts("\ntesting set functionality");

    puts("creating table");
    table = glh_new(hash_func, equal_func);
    assert(table);
    assert( 32 == table->size );
    assert( 0 == glh_nelems(table) );


    puts("inserting some data");
    assert( glh_insert(table, key_1, &data_1) );
    assert( 1 == glh_nelems(table) );
    assert( 0 == glh_get(table, key_2) );
    assert( 0 == glh_get(table, key_3) );

    data = glh_get(table, key_1);
    assert(data);
    assert( data_1 == *data );


    assert( glh_insert(table, key_2, &data_2) );
    assert( 2 == glh_nelems(table) );
    assert( 0 == glh_get(table, key_3) );

    data = glh_get(table, key_2);
    assert(data);
    assert( data_2 == *data );


    assert( glh_insert(table, key_3, &data_3) );
    assert( 3 == glh_nelems(table) );

    data = glh_get(table, key_3);
    assert(data);
    assert( data_3 == *data );


    puts("testing set");
    puts("testing set failure for non-existing key");
    data = glh_set(table, "foobarr", &data_1);
    assert( 0 == data );

    puts("two set");
    data = glh_set(table, key_2, &new_data_2);
    assert(data);
    assert( *data == data_2 );
    assert( 3 == glh_nelems(table) );

    data = glh_get(table, key_2);
    assert(data);
    assert( *data == new_data_2 );

    puts("three set");
    data = glh_set(table, key_3, &new_data_3);
    assert(data);
    assert( *data == data_3 );
    assert( 3 == glh_nelems(table) );

    data = glh_get(table, key_3);
    assert(data);
    assert( *data == new_data_3 );

    puts("one set");
    data = glh_set(table, key_1, &new_data_1);
    assert(data);
    assert( *data == data_1 );
    assert( 3 == glh_nelems(table) );

    data = glh_get(table, key_1);
    assert(data);
    assert( *data == new_data_1 );


    assert( glh_destroy(table, 1, 0) );
    puts("success!");
}

void delete(void){
    /* our simple hash table */
    struct glh_table *table = 0;

    /* some keys */
    char *key_1 = "bacon";
    char *key_2 = "chicken";
    char *key_3 = "pork";

    /* some data */
    int data_1 = 1;
    int data_2 = 2;
    int data_3 = 3;

    /* temporary data pointer used for testing get */
    int *data = 0;

    puts("\ntesting delete functionality ");

    puts("creating a table");
    table = glh_new(hash_func, equal_func);
    assert(table);
    assert( 32 == table->size );
    assert( 0 == glh_nelems(table) );


    puts("inserting some data");
    assert( glh_insert(table, key_1, &data_1) );
    assert( 1 == glh_nelems(table) );
    assert( 0 == glh_get(table, key_2) );
    assert( 0 == glh_get(table, key_3) );

    data = glh_get(table, key_1);
    assert(data);
    assert( data_1 == *data );


    assert( glh_insert(table, key_2, &data_2) );
    assert( 2 == glh_nelems(table) );
    assert( 0 == glh_get(table, key_3) );

    data = glh_get(table, key_2);
    assert(data);
    assert( data_2 == *data );


    assert( glh_insert(table, key_3, &data_3) );
    assert( 3 == glh_nelems(table) );

    data = glh_get(table, key_3);
    assert(data);
    assert( data_3 == *data );


    puts("testing delete");
    puts("one delete");
    data = glh_delete(table, key_1);
    assert(data);
    assert(*data == data_1);
    assert( 2 == glh_nelems(table) );

    /* should not be able to re-delete */
    data = glh_delete(table, key_1);
    assert(! data);


    puts("two delete");
    data = glh_delete(table, key_3);
    assert(data);
    assert(*data == data_3);
    assert( 1 == glh_nelems(table) );

    /* should not be able to re-delete */
    data = glh_delete(table, key_3);
    assert(! data);


    puts("three delete");
    data = glh_delete(table, key_2);
    assert(data);
    assert(*data == data_2);
    assert( 0 == glh_nelems(table) );

    /* should not be able to re-delete */
    data = glh_delete(table, key_2);
    assert(! data);


    assert( glh_destroy(table, 1, 0) );
    puts("success!");
}

void collision(void){
    /* our simple hash table */
    struct glh_table *table = 0;

    /* some keys */
    char *key_1 = "bacon";
    char *key_2 = "chicken";
    char *key_3 = "pork";
    char *key_4 = "pig";
    char *key_5 = "lettuce";
    char *key_6 = "beetroot";
    char *key_7 = "chocolate";
    /* long string to try cause some havoc */
    char *key_8 = "frying pan porcupine";
    char *key_9 = "a4 paper";

    /* some data */
    int data_1 = 1;
    int data_2 = 2;
    int data_3 = 3;
    int data_4 = 4;
    int data_5 = 5;
    int data_6 = 6;
    int data_7 = 7;
    int data_8 = 8;
    int data_9 = 9;

    /* temporary data pointer used for testing get */
    int *data = 0;

    puts("\ntesting collision behavior ");

    puts("creating a table");
    table = glh_new(hash_func, equal_func);
    assert(table);
    assert( 32 == table->size );
    assert( 0 == glh_nelems(table) );
    assert( 0 == glh_load(table) );

    /* artificially shrink down table */
    assert( glh_resize(table, 9) );
    assert( 9 == table->size );
    assert( 0 == glh_nelems(table) );
    assert( 0 == glh_load(table) );


    puts("inserting some data");
    assert( glh_insert(table, key_1, &data_1) );
    assert( 1 == glh_nelems(table) );
    data = glh_get(table, key_1);
    assert(data);
    assert( data_1 == *data );


    assert( glh_insert(table, key_2, &data_2) );
    assert( 2 == glh_nelems(table) );
    data = glh_get(table, key_2);
    assert(data);
    assert( data_2 == *data );


    assert( glh_insert(table, key_3, &data_3) );
    assert( 3 == glh_nelems(table) );
    data = glh_get(table, key_3);
    assert(data);
    assert( data_3 == *data );

    assert( glh_insert(table, key_4, &data_4) );
    assert( 4 == glh_nelems(table) );
    data = glh_get(table, key_4);
    assert(data);
    assert( data_4 == *data );

    assert( glh_insert(table, key_5, &data_5) );
    assert( 5 == glh_nelems(table) );
    data = glh_get(table, key_5);
    assert(data);
    assert( data_5 == *data );

    assert( glh_insert(table, key_6, &data_6) );
    assert( 6 == glh_nelems(table) );
    data = glh_get(table, key_6);
    assert(data);
    assert( data_6 == *data );

    assert( glh_insert(table, key_7, &data_7) );
    assert( 7 == glh_nelems(table) );
    data = glh_get(table, key_7);
    assert(data);
    assert( data_7 == *data );

    assert( glh_insert(table, key_8, &data_8) );
    assert( 8 == glh_nelems(table) );
    data = glh_get(table, key_8);
    assert(data);
    assert( data_8 == *data );

    assert( glh_insert(table, key_9, &data_9) );
    assert( 9 == glh_nelems(table) );
    data = glh_get(table, key_9);
    assert(data);
    assert( data_9 == *data );

    assert( 9 == glh_nelems(table) );
    /* assert that our resize succeeded and we
     * are not full */
    assert( 10 > glh_load(table) );
    assert( 9 < table->size );

    puts("testing we can still get everything out");

    data = glh_get(table, key_3);
    assert(data);
    assert( data_3 == *data );

    data = glh_get(table, key_1);
    assert(data);
    assert( data_1 == *data );

    data = glh_get(table, key_2);
    assert(data);
    assert( data_2 == *data );

    data = glh_get(table, key_4);
    assert(data);
    assert( data_4 == *data );

    data = glh_get(table, key_8);
    assert(data);
    assert( data_8 == *data );

    data = glh_get(table, key_5);
    assert(data);
    assert( data_5 == *data );

    data = glh_get(table, key_6);
    assert(data);
    assert( data_6 == *data );

    data = glh_get(table, key_7);
    assert(data);
    assert( data_7 == *data );

    data = glh_get(table, key_9);
    assert(data);
    assert( data_9 == *data );


    puts("testing delete collision handling");

    data = glh_delete(table, key_1);
    assert(data);
    assert(*data == data_1);
    assert( 8 == glh_nelems(table) );

    /* should not be able to re-delete */
    data = glh_delete(table, key_1);
    assert(! data);


    data = glh_delete(table, key_3);
    assert(data);
    assert(*data == data_3);
    assert( 7 == glh_nelems(table) );

    /* should not be able to re-delete */
    data = glh_delete(table, key_3);
    assert(! data);


    data = glh_delete(table, key_2);
    assert(data);
    assert(*data == data_2);
    assert( 6 == glh_nelems(table) );

    /* should not be able to re-delete */
    data = glh_delete(table, key_2);
    assert(! data);

    puts("checking post-delete that all items that should be reachable are");

    data = glh_get(table, key_5);
    assert(data);
    assert( data_5 == *data );

    data = glh_get(table, key_8);
    assert(data);
    assert( data_8 == *data );

    data = glh_get(table, key_4);
    assert(data);
    assert( data_4 == *data );

    data = glh_get(table, key_6);
    assert(data);
    assert( data_6 == *data );

    data = glh_get(table, key_9);
    assert(data);
    assert( data_9 == *data );

    data = glh_get(table, key_7);
    assert(data);
    assert( data_7 == *data );

    assert( glh_destroy(table, 1, 0) );
    puts("success!");
}

void resize(void){
    /* our simple hash table */
    struct glh_table *table = 0;

    /* some keys */
    char *key_1 = "bbbbb";
    char *key_2 = "aaaaa";
    char *key_3 = "ccccc";

    /* some data */
    int data_1 = 1;
    int data_2 = 2;
    int data_3 = 3;

    /* temporary data pointer used for testing get */
    int *data = 0;


    puts("\ntesting resize functionality");

    puts("creating table");
    table = glh_new(hash_func, equal_func);
    assert(table);
    assert( 32 == table->size );
    assert( 0 == glh_nelems(table) );

    /* artificially shrink down */
    assert( glh_resize(table, 3) );
    assert( 3 == table->size );
    assert( 0 == glh_nelems(table) );

    puts("inserting some data");
    assert( glh_insert(table, key_1, &data_1) );
    assert( 1 == glh_nelems(table) );
    assert( 0 == glh_get(table, key_2) );
    assert( 0 == glh_get(table, key_3) );
    data = glh_get(table, key_1);
    assert(data);
    assert( data_1 == *data );


    assert( glh_insert(table, key_2, &data_2) );
    assert( 2 == glh_nelems(table) );
    assert( 0 == glh_get(table, key_3) );
    data = glh_get(table, key_2);
    assert(data);
    assert( data_2 == *data );


    assert( glh_insert(table, key_3, &data_3) );
    assert( 3 == glh_nelems(table) );
    data = glh_get(table, key_3);
    assert(data);
    assert( data_3 == *data );


    puts("testing resize");
    assert( glh_resize(table, 10) );
    assert( 3  == glh_nelems(table) );
    assert( 10 == table->size );

    puts("testing we can still fetch all the old values");
    data = glh_get(table, key_3);
    assert(data);
    assert( data_3 == *data );

    data = glh_get(table, key_2);
    assert(data);
    assert( data_2 == *data );

    data = glh_get(table, key_1);
    assert(data);
    assert( data_1 == *data );

    assert( glh_destroy(table, 1, 0) );
    puts("success!");
}

void destroy(void){
    /* specifically test glh_destroy with free_data = 1 */

    /* our simple hash table */
    struct glh_table *table = 0;

    /* some keys */
    char *key_1 = "bbbbb";
    char *key_2 = "aaaaa";
    char *key_3 = "ccccc";

    /* some data */
    int *data_1 = 0;
    int *data_2 = 0;
    int *data_3 = 0;

    /* temporary data pointer used for testing get */
    int *data = 0;

    data_1 = calloc(1, sizeof(int));
    assert( data_1 );
    *data_1 = 1;

    data_2 = calloc(1, sizeof(int));
    assert( data_2 );
    *data_2 = 2;

    data_3 = calloc(1, sizeof(int));
    assert( data_3 );
    *data_3 = 3;


    puts("\ntesting destroy");

    puts("creating table");
    table = glh_new(hash_func, equal_func);
    assert(table);
    assert( 32 == table->size );
    assert( 0 == glh_nelems(table) );


    puts("populating");
    assert( glh_insert(table, key_1, data_1) );
    assert( 1 == glh_nelems(table) );
    assert( 0 == glh_get(table, key_2) );
    assert( 0 == glh_get(table, key_3) );
    data = glh_get(table, key_1);
    assert(data);
    assert( *data_1 == *data );


    assert( glh_insert(table, key_2, data_2) );
    assert( 2 == glh_nelems(table) );
    assert( 0 == glh_get(table, key_3) );
    data = glh_get(table, key_2);
    assert(data);
    assert( *data_2 == *data );


    assert( glh_insert(table, key_3, data_3) );
    assert( 3 == glh_nelems(table) );
    data = glh_get(table, key_3);
    assert(data);
    assert( *data_3 == *data );


    /* destroy including all data */
    assert( glh_destroy(table, 1, 1) );
    puts("success!");
}

void error_handling(void){
    /* our simple hash table */
    struct glh_table *table = 0;
    struct glh_table *not_table = 0;
    struct glh_table static_table;

    /* some keys */
    char *key_1 = "bbbbb";
    char *key_2 = "aaaaa";
    char *key_3 = "ccccc";

    /* some data */
    int data_1 = 1;
    int data_2 = 2;
    int data_3 = 3;

    /* temporary data pointer used for testing get */
    int *data = 0;


    puts("\ntesting handling of error cases");

    puts("testing glh_nelems");
    assert( 0 == glh_nelems(0) );

    puts("setting up...");

    puts("creating table");
    table = glh_new(hash_func, equal_func);
    assert(table);
    assert( 32 == table->size );
    assert( 0 == glh_nelems(table) );

    /* artificially shrink */
    assert( glh_resize(table, 3) );
    assert( 3 == table->size );
    assert( 0 == glh_nelems(table) );

    puts("inserting some data");
    assert( glh_insert(table, key_1, &data_1) );
    assert( 1 == glh_nelems(table) );
    assert( 0 == glh_get(table, key_2) );
    assert( 0 == glh_get(table, key_3) );
    data = glh_get(table, key_1);
    assert(data);
    assert( data_1 == *data );


    assert( glh_insert(table, key_2, &data_2) );
    assert( 2 == glh_nelems(table) );
    assert( 0 == glh_get(table, key_3) );
    data = glh_get(table, key_2);
    assert(data);
    assert( data_2 == *data );


    puts("beginning actual testing, cue wall of errors");

    /* glh_init */
    puts("testing glh_init");
    assert( 0 == glh_init(0, 100, hash_func, equal_func) );
    assert( 0 == glh_init(&static_table, 0, hash_func, equal_func) );

    /* glh_resize */
    puts("testing glh_resize");
    assert( 0 == glh_resize(0, 100) );
    assert( 0 == glh_resize(table, 0) );

    /* glh_exists */
    puts("testing glh_exists");
    assert( 0 == glh_exists(0, key_1) );
    assert( 0 == glh_exists(table, 0) );
    assert( 0 == glh_exists(table, key_3) );

    /* glh_insert */
    puts("testing glh_insert");
    assert( 0 == glh_insert(0, key_1, &data_1) );
    assert( 0 == glh_insert(table, 0, &data_1) );
    /* cannot insert if already exists */
    assert( 0 == glh_insert(table, key_1, &data_1) );

    /* glh_set */
    puts("testing glh_set");
    assert( 0 == glh_set(0, key_1, &data_1) );
    assert( 0 == glh_set(table, 0, &data_1) );
    /* cannot set if doesn't already exist */
    assert( 0 == glh_set(table, key_3, &data_3) );

    /* glh_get */
    puts("testing glh_get");
    assert( 0 == glh_get(0, key_1) );
    assert( 0 == glh_get(table, 0) );
    assert( 0 == glh_get(table, key_3) );

    /* glh_delete */
    puts("testing glh_delete");
    assert( 0 == glh_delete(0, key_1) );
    assert( 0 == glh_delete(table, 0) );
    /* cannot delete a non-existent key */
    assert( 0 == glh_delete(table, key_3) );

    /* glh_tune_threshold */
    puts("testing glh_tune_threshold");
    assert( 0 == glh_tune_threshold(0, 0) );
    assert( 0 == glh_tune_threshold(table, 0) );
    assert( 0 == glh_tune_threshold(table, 11) );

    /* glh_destroy */
    assert( 0 == glh_destroy(0, 1, 0) );

    /* tidy up */
    assert( glh_destroy(table, 1, 0) );
    puts("success!");
}

void internal(void){
    struct glh_table table;
    struct glh_entry she;
    struct glh_entry static_she;
    char * str = 0;

    puts("\ntesting internal functions");

    puts("testing glh_new error handling");
    assert( 0 == glh_new(0, 0) );

    puts("testing glh_init error handling");
    assert( 0 == glh_init(&table, 32, 0, 0) );
    /* actually build table for following tests */
    assert( glh_init(&table, 32, hash_func, 0) );

    /* glh_load */
    puts("testing glh_load error handling");
    assert( 0 == glh_load(0) );

    /* glh_entry_new and glh_entry_init */
    puts("testing glh_entry_new and glh_entry_init");
    assert( 0 == glh_entry_init(0, 0, 0, 0, 0) );
    assert( 0 == glh_entry_init(&table, 0, 0, 0, 0) );
    assert( 0 == glh_entry_init(&table, &she, 0, 0, 0) );
    assert( glh_entry_init(&table, &static_she, 0, "hello", 0) );

    /* glh_entry_destroy */
    puts("testing glh_entry_destroy");
    assert( 0 == glh_entry_destroy(0, 0) );
    static_she.data = calloc(1, sizeof(int));
    assert(static_she.data);
    assert(  glh_entry_destroy(&static_she, 1) );

    /* glh_find_entry */
    puts("testing glh_find_entry");
    assert( 0 == glh_find_entry(0, "hello") );
    assert( 0 == glh_find_entry(&table, 0) );

    /* glh_entry_eq */
    puts("testing glh_entry_eq");
    assert( 0 == glh_entry_eq(&table, 0, 0, 0) );
    assert( 0 == glh_entry_eq(&table, &she, 0, 0) );
    assert( 0 == glh_entry_eq(0, &she, 0, 0) );

    puts("success!");
}

void load_resize(void){
    /* our simple hash table */
    struct glh_table *table = 0;

    /* some keys */
    char *key_1 = "bbbbb";
    char *key_2 = "aaaaa";
    char *key_3 = "ccccc";
    char *key_4 = "ddddd";

    /* some data */
    int data_1 = 1;
    int data_2 = 2;
    int data_3 = 3;
    int data_4 = 4;

    /* temporary data pointer used for testing get */
    int *data = 0;


    puts("\ntesting auto resizing at load");

    puts("testing new");
    table = glh_new(hash_func, equal_func);
    assert(table);
    assert( 32 == table->size );
    assert( 0 == glh_nelems(table) );
    assert( 0 == glh_load(table) );

    /* artificially shrink */
    assert( glh_resize(table, 4) );
    assert( 4 == table->size );
    assert( 0 == glh_nelems(table) );
    assert( 0 == glh_load(table) );

    puts("adding some data to force a resize");
    assert( glh_insert(table, key_1, &data_1) );
    assert( 1 == glh_nelems(table) );
    assert( 0 == glh_get(table, key_2) );
    assert( 0 == glh_get(table, key_3) );
    data = glh_get(table, key_1);
    assert(data);
    assert( data_1 == *data );

    /* insert tests resize before inserting
     * so from it's view at that time:
     * 0 / 4 = 0 % loading
     * no resize
     */
    assert( 4 == table->size );
    assert( 1 == glh_nelems(table) );
    /* however the load now will be
     * 1 / 4 = 25 %
     */
    assert( 2 == glh_load(table) );


    assert( glh_insert(table, key_2, &data_2) );
    assert( 2 == glh_nelems(table) );
    assert( 0 == glh_get(table, key_3) );
    data = glh_get(table, key_2);
    assert(data);
    assert( data_2 == *data );

    /* insert tests resize before inserting
     * so from it's view at that time:
     * 1 / 4 = 25 % loading
     * no resize
     */
    assert( 4 == table->size );
    assert( 2 == glh_nelems(table) );
    /* however the load now will be
     * 2 / 4 = 50 %
     */
    assert( 5 == glh_load(table) );


    assert( glh_insert(table, key_3, &data_3) );
    assert( 3 == glh_nelems(table) );
    data = glh_get(table, key_3);
    assert(data);
    assert( data_3 == *data );

    /* insert tests resize before inserting
     * so from it's view at that time:
     * 2 / 4 = 50 % loading
     * no resize
     */
    assert( 4 == table->size );
    assert( 3 == glh_nelems(table) );
    /* however the load now will be
     * 3 / 4 = 70 %
     * so this will trigger a resize (as 70 >= 60)
     */
    assert( 7 == glh_load(table) );


    assert( glh_insert(table, key_4, &data_4) );
    assert( 4 == glh_nelems(table) );
    data = glh_get(table, key_4);
    assert(data);
    assert( data_4 == *data );

    /* insert tests resize before inserting
     * so from it's view at that time:
     * 3 / 4 = 75 % loading
     * so resize called to double
     */
    assert( 8 == table->size );
    assert( 4 == glh_nelems(table) );
    /* and after resizing the load now will be
     * 4 / 8 = 50 %
     */
    assert( 5 == glh_load(table) );


    assert( glh_destroy(table, 1, 0) );
    puts("success!");

}

void rollover(void){
    struct glh_table *table = 0;

    /* with length 4 and key 'c' we can hit the 3rd spot */
    char *key = "c";
    int data = 1;

    /* with length 4 we know that both 'c' and 'g' will hit the 3rd spot */
    char *key_2 = "g";
    int data_2 = 2;

    int *d = 0;

    puts("\ntesting rollover");

    puts("making table");
    table = glh_new(hash_func, equal_func);
    assert(table);

    /* force shrink */
    assert( glh_resize(table, 4) );
    assert( 4 == table->size );

    /* mark 3rd position as occupied as we know
     * that our key would end up there
     * the intention here is to force the second for loop
     * for insert and delete to activate
     */
    table->entries[3].state = glh_ENTRY_OCCUPIED;

    /* force insert to roll over */
    assert( glh_insert(table, key, &data) );

    /* force delete to roll over */
    assert( glh_delete(table, key) );

    /* make sure to remark as empty */
    table->entries[3].state = glh_ENTRY_EMPTY;

    /* force resize to 2 */
    assert( glh_resize(table, 2) );
    assert( 2 == table->size );
    assert( 0 == glh_nelems(table) );

    /* insert 2 elements that we know will collide */
    assert( glh_insert(table, key, &data) );
    assert( glh_insert(table, key_2, &data) );
    /* no resize yet */
    assert( 2 == table->size );

    /* force a resize, this will move to 4
     * and this will trigger the second for
     * loop in glh_resize
     */
    assert( glh_resize(table, 4) );


    /* cleanup */
    assert( glh_destroy(table, 1, 0) );
}

void threshold(void){
    struct glh_table *table = 0;
    int data = 1;

    puts("\ntesting lh load threshold setting");

    puts("building table");
    table = glh_new(hash_func, equal_func);
    assert(table);

    /* shrink artificially */
    assert( glh_resize(table, 4) );

    /* tune to only resize at 100% */
    assert( glh_tune_threshold(table, 10) );
    assert( 10 == table->threshold );

    /* insert 4 times checking there has been no resizing */
    assert( glh_insert(table, "a", &data) );
    assert( 1 == glh_nelems(table) );
    assert( 4 == table->size );

    assert( glh_insert(table, "b", &data) );
    assert( 2 == glh_nelems(table) );
    assert( 4 == table->size );

    assert( glh_insert(table, "c", &data) );
    assert( 3 == glh_nelems(table) );
    assert( 4 == table->size );

    assert( glh_insert(table, "d", &data) );
    assert( 4 == glh_nelems(table) );
    assert( 4 == table->size );

    /* insert 1 more and check resize */
    assert( glh_insert(table, "e", &data) );
    assert( 5 == glh_nelems(table) );
    assert( 8 == table->size );
    assert( 10 == table->threshold );

    assert( glh_destroy(table, 1, 0) );
    puts("success!");
}

void artificial(void){
    struct glh_table *table = 0;
    int data = 1;

    puts("\ntesting artificial linear search failure");

    puts("building table");
    table = glh_new(hash_func, equal_func);
    assert(table);

    /* shrink artificially */
    assert( glh_resize(table, 2) );

    puts("manipulating and testing of find and insert");
    /* fill only positions */
    table->entries[0].state = glh_ENTRY_OCCUPIED;
    table->entries[1].state = glh_ENTRY_OCCUPIED;

    /* this should trigger the final return 0 of glh_find_entry */
    assert( 0 == glh_find_entry(table, "hello") );

    /* likewise an insert should fail */
    assert( 0 == glh_insert(table, "c", &data) );

    puts("further manipulation for glh_find_entry");
    table->entries[0].state = glh_ENTRY_DUMMY;
    table->entries[1].state = glh_ENTRY_DUMMY;
    assert( 0 == glh_find_entry(table, "c") );

    table->entries[0].state   = glh_ENTRY_OCCUPIED;
    table->entries[0].hash    = 99; /* force hash collision with c */
    table->entries[0].key     = "a";
    table->entries[1].state   = glh_ENTRY_OCCUPIED;
    table->entries[1].hash    = 99; /* force hash collision with c */
    table->entries[1].key     = "b"; /* but different key */
    assert( 0 == glh_find_entry(table, "c") );

    assert( glh_resize(table, 3) );
    table->entries[0].state   = glh_ENTRY_OCCUPIED;
    table->entries[0].hash    = 98; /* force hash collision with b */
    table->entries[0].key     = "a";

    table->entries[1].state   = glh_ENTRY_OCCUPIED;
    table->entries[1].hash    = 98; /* force hash collision with b */
    table->entries[1].key     = "a"; /* but different key */

    table->entries[2].state   = glh_ENTRY_DUMMY;
    assert( 0 == glh_find_entry(table, "b") );

    puts("manipulation to provoke glh_resize");
    assert( glh_resize(table, 5) );
    /* provoke 3 hash collision on a resize
     * where the preferred pos is the final element
     *
     * we also secretly fill up the elements without
     * increment n_elems
     *
     * we then resize so that there is not enough room
     * this is only possible with manipulation as insert
     * will increase n_elems and resize will refuse to
     * shrink below n_elems
     */
    table->entries[0].state   = glh_ENTRY_OCCUPIED;
    table->entries[0].hash    = 101;
    table->entries[1].state   = glh_ENTRY_OCCUPIED;
    table->entries[1].hash    = 101;
    table->entries[2].state   = glh_ENTRY_OCCUPIED;
    table->entries[2].hash    = 101;
    table->entries[3].state   = glh_ENTRY_OCCUPIED;
    table->entries[3].hash    = 101;
    table->entries[4].state   = glh_ENTRY_OCCUPIED;
    table->entries[4].hash    = 101;
    /* we won't be able to fit everything in! */
    assert( 0 ==  glh_resize(table, 4) );

    puts("manipulating to torture glh_delete");

    /*
     * examples of some useful keys in our length
     * 4 table
     *
     * glh_pos(hash_func(X), 4) == Y
     *
     * for key  X  we get hash  Y
     *
     *  for key 'a' we get hash '1'
     *  for key 'b' we get hash '2'
     *  for key 'c' we get hash '3'
     *  for key 'd' we get hash '0'
     *  for key 'e' we get hash '1'
     *  for key 'f' we get hash '2'
     *  for key 'g' we get hash '3'
     */

    table->entries[4].state   = glh_ENTRY_EMPTY;
    assert( glh_resize(table, 4) );

    table->entries[0].state   = glh_ENTRY_OCCUPIED;
    table->entries[0].hash    = 99; /* hash collide with c */

    table->entries[1].state   = glh_ENTRY_OCCUPIED;
    table->entries[1].hash    = 99;
    table->entries[1].key     = "z"; /* different key */

    table->entries[2].state   = glh_ENTRY_DUMMY;

    table->entries[3].state   = glh_ENTRY_OCCUPIED;
    table->entries[3].hash    = 99;

    assert( 0 == glh_delete(table, "c") );

    table->entries[0].state   = glh_ENTRY_OCCUPIED;
    table->entries[0].hash    = 1; /* hash differ */

    table->entries[1].state   = glh_ENTRY_OCCUPIED;
    table->entries[1].hash    = 100; /* hash collide with d */

    table->entries[2].state   = glh_ENTRY_OCCUPIED;
    table->entries[2].hash    = 100;
    table->entries[2].key     = "z"; /* different key */

    table->entries[3].state   = glh_ENTRY_DUMMY;

    assert( 0 == glh_delete(table, "d") );

    /* we want to force wrap around
     * and then encounter an empty
     */
    table->entries[0].state   = glh_ENTRY_EMPTY;

    table->entries[1].state   = glh_ENTRY_OCCUPIED;
    table->entries[1].hash    = 99; /* hash collide with c */
    table->entries[1].key     = "z"; /* different key */

    table->entries[2].state   = glh_ENTRY_OCCUPIED;
    table->entries[2].hash    = 99; /* hash collide with c */
    table->entries[2].key     = "z"; /* different key */

    table->entries[3].state   = glh_ENTRY_OCCUPIED;
    table->entries[3].hash    = 99; /* hash collide with c */
    table->entries[3].key     = "z"; /* different key */

    assert( 0 == glh_delete(table, "c") );


    puts("success!");
}

int main(void){
    new_insert_get_destroy();

    set();

    delete();

    collision();

    resize();

    destroy();

    error_handling();

    internal();

    load_resize();

    rollover();

    threshold();

    artificial();

    puts("\noverall testing success!");

    return 0;
}
