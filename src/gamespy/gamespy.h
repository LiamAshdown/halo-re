// gamespy.h -- shared declarations for the GameSpy SDK linked into halo.exe (0x614540..0x623142, "lib:gamespy").
// Names follow the public GameSpy SDK where the code, strings and structure layouts match it (darray.c,
// hashtable.c, nonport.c, qr2, serverbrowsing, gcdkey, ghttp); every function is cdecl unless its header says
// otherwise. The SDK calls the C runtime and Winsock directly, so these files do too (ws2_32.lib is linked).

#ifndef HALO_GAMESPY_H
#define HALO_GAMESPY_H

#include "win32.h"   /* the Windows SDK (winsock2 / windows.h), shared with the game code */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

// ---- darray.c
typedef void (*ArrayElementFreeFn)(void *elem);
typedef int (*ArrayCompareFn)(const void *elem1, const void *elem2);
typedef void (*ArrayMapFn)(void *elem, void *client_data);
typedef int (*ArrayMapFn2)(void *elem, void *client_data); // returns 0 to stop

typedef struct DArrayImplementation {
    int count;                      // 0x00
    int capacity;                   // 0x04
    int elemsize;                   // 0x08
    int growby;                     // 0x0c
    ArrayElementFreeFn elemfreefn;  // 0x10
    void *list;                     // 0x14
} DArrayImplementation, *DArray;    // size 0x18

DArray ArrayNew(int elem_size, int grow_by, ArrayElementFreeFn elem_free_fn);
void ArrayFree(DArray array);
void *ArrayNth(DArray array, int n);
void ArrayInsertAt(DArray array, const void *new_elem, int n);
void ArrayInsertSorted(DArray array, const void *new_elem, ArrayCompareFn comparator);
void ArrayAppend(DArray array, const void *new_elem);
void ArrayRemoveAt(DArray array, int n);
void ArrayDeleteAt(DArray array, int n);
void ArrayReplaceAt(DArray array, const void *new_elem, int n);
int ArraySearch(DArray array, const void *key, ArrayCompareFn comparator, int from_index, int is_sorted);
void ArrayMap(DArray array, ArrayMapFn fn, void *client_data);
void ArrayMapBackwards(DArray array, ArrayMapFn fn, void *client_data);
void *ArrayMapBackwards2(DArray array, ArrayMapFn2 fn, void *client_data);
void ArrayClear(DArray array);

// ---- hashtable.c
typedef int (*TableHashFn)(const void *elem, int num_buckets);

typedef struct HashImplementation {
    DArray *buckets;                // 0x00
    int nbuckets;                   // 0x04
    ArrayElementFreeFn freefn;      // 0x08
    TableHashFn hashfn;             // 0x0c
    ArrayCompareFn compfn;          // 0x10
} HashImplementation, *HashTable;   // size 0x14

HashTable TableNew2(int elem_size, int num_buckets, int num_chains_per_bucket, TableHashFn hash_fn,
    ArrayCompareFn comp_fn, ArrayElementFreeFn free_fn);
void TableFree(HashTable table);
void TableEnter(HashTable table, const void *new_elem);
int TableRemove(HashTable table, const void *del_elem);
void *TableLookup(HashTable table, const void *elem);
void TableMap(HashTable table, ArrayMapFn fn, void *client_data);
void TableMapSafe(HashTable table, ArrayMapFn fn, void *client_data);
void *TableMapSafe2(HashTable table, ArrayMapFn2 fn, void *client_data);

// ---- nonport.c
unsigned long current_time(void);
void msleep(unsigned long msec);
void SocketStartUp(void);
void SocketShutDown(void);
char *goastrdup(const char *src);
int SetSockBlocking(SOCKET sock, int is_blocking);
int SetReceiveBufferSize(SOCKET sock, int size);
int CanReceiveOnSocket(SOCKET sock);

int ArrayLength(DArray array);
int TableCount(HashTable table);

// ---- sb_server.c (serverbrowsing)
typedef struct SBKeyValuePair {
    const char *key;                // 0x00
    const char *value;              // 0x04
} SBKeyValuePair;

int KeyValCompareKeyA(const void *elem1, const void *elem2);
int KeyValHashKeyA(const void *elem, int num_buckets);
void SBRefStrFree(void *elem);
HashTable SBRefStrHash(void *slist);
const char *SBRefStr(void *slist, const char *str);
void SBReleaseStr(void *slist, const char *str);
int NTSLengthSB(const char *buf, int len);
void SBServerKeyValFree(void *elem);
typedef void (*SBListCallBackFn)(void *slist, int reason, void *server, void *instance);

#endif
