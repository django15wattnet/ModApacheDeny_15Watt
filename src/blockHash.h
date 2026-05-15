#ifndef MODAPACHEDENY_15WATT_BLOCKHASH_H
#define MODAPACHEDENY_15WATT_BLOCKHASH_H
#include <stdbool.h>
#include <apr_pools.h>
#include <apr_hash.h>
#include <apr_proc_mutex.h>
#include <apr_shm.h>
#include <httpd.h>

// List of reasons an entry has been blocked
enum EnumBlockType {
    blockTypeNotFound           = 0,
    blockTypeNone               = 1,
    blockTypeIpV4               = 2,
    blockTypeIpV6               = 3,
    blockTypeCidrIpV4           = 4,
    blockTypeCidrIpV6           = 5,
    blockTypeUserAgent          = 6,
};

extern const char* BlockTypeStrings[];

// An entry in the block list
typedef struct {
    apr_pool_t         *pool;
    bool               doBlock;
    enum EnumBlockType blockType;
    int                tsLastUse;
    const char         *key;
} BlockHashEntry;

// Structure to hold the block list and all necessary informations
typedef struct {
    apr_hash_t         *blockHash;
    int                maxEntries;
    apr_pool_t         *serverPool;
    apr_proc_mutex_t   *mutex;
    apr_shm_t          *shm;
} BlockHash;

extern BlockHash blockHash;

bool blockHashSetUpStore(const server_rec *serverRec, const int maxEntries);
bool blockHashRemoveOldestEntry();
BlockHashEntry *blockHashGetEntry(const char *key);
int blockHashAddEntry(const char *key, const enum EnumBlockType blockType, const bool doBlock, apr_pool_t *parentPool);
int blockHashGetEntryCount();
void blockHashGetOldestAndNewestEntries(BlockHashEntry* oldest_entries[10], int* num_oldest, BlockHashEntry* newest_entries[10], int* num_newest);

#endif // MODAPACHEDENY_15WATT_BLOCKHASH_H