#ifndef MODAPACHEDENY_15WATT_BLOCKHASH_H
#define MODAPACHEDENY_15WATT_BLOCKHASH_H
#include <stdbool.h>
#include <apr_pools.h>
#include <apr_proc_mutex.h>
#include <apr_shm.h>
#include <httpd.h>

// Shared memory and mutex paths for inter-process communication
#define BLOCK_HASH_SHM_PATH "/tmp/modapachedeny_15watt_blockhash_shm"
#define BLOCK_HASH_MUTEX_PATH "/tmp/modapachedeny_15watt_blockhash_mutex"

// Hash table tuning constants
#define BLOCK_HASH_MAX_CAPACITY 20000      // Maximum number of entries
#define BLOCK_HASH_BUCKET_COUNT 4096       // Number of hash buckets (separate chaining)
#define BLOCK_HASH_KEY_MAX_LEN 255         // Maximum key length
#define BLOCK_HASH_INDEX_NONE -1           // Sentinel value for linked list termination

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

extern const char *BlockTypeStrings[];

// Public view of an entry in the block list
typedef struct {
    bool               doBlock;
    enum EnumBlockType blockType;
    int                tsLastUse;
    int                cnt;
    const char         *key;
} BlockHashEntry;

// Structure to hold the block list runtime state
typedef struct {
    void               *blockHash;   // debug/status pointer to shared store
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
unsigned long blockHashGetSharedStoreSizeKb();
void blockHashGetOldestAndNewestEntries(BlockHashEntry *oldest_entries[10], int *num_oldest, BlockHashEntry *newest_entries[10], int *num_newest);

#endif // MODAPACHEDENY_15WATT_BLOCKHASH_H
