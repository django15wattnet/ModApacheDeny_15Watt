#include <apr_shm.h>
#include <apr_proc_mutex.h>
#include <apr_errno.h>
#include <apr_file_io.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "blockHash.h"

#include <httpd.h>
#include <http_log.h>


const char *BlockTypeStrings[] = {
    "Not Found",
    "None",
    "IPv4",
    "IPv6",
    "CIDR IPv4",
    "CIDR IPv6",
    "User Agent"
};

// Global runtime handle (contains shm/mutex handles and a debug pointer to the shared store)
BlockHash blockHash;

typedef struct {
    int inUse;
    int next;
    unsigned int keyHash;
    int tsLastUse;
    int cnt;
    unsigned char doBlock;
    unsigned char blockType;
    char key[BLOCK_HASH_KEY_MAX_LEN + 1];
} SharedBlockHashEntry;

// Shared-memory payload: fixed-size hash table with separate chaining.
typedef struct {
    int initialized;
    int entryCount;
    int maxEntries;
    int buckets[BLOCK_HASH_BUCKET_COUNT];
    SharedBlockHashEntry entries[BLOCK_HASH_MAX_CAPACITY];
} SharedBlockHashStore;

static SharedBlockHashStore *getSharedStore()
{
    if (NULL == blockHash.shm) {
        return NULL;
    }
    return (SharedBlockHashStore *)apr_shm_baseaddr_get(blockHash.shm);
}

// Lightweight stable hash for key distribution across buckets.
static unsigned int keyHashDjb2(const char *str)
{
    unsigned int hash = 5381U;
    unsigned char c;

    while ((c = (unsigned char)*str++) != 0U) {
        hash = ((hash << 5U) + hash) + c;
    }

    return hash;
}

// Finds an entry index in the chained bucket list.
static int findEntryIndex(
    const SharedBlockHashStore *store,
    const char *key,
    const unsigned int hash,
    int *bucketOut,
    int *prevOut
)
{
    const int bucket = (int)(hash % BLOCK_HASH_BUCKET_COUNT);
    int prev = BLOCK_HASH_INDEX_NONE;
    int idx = store->buckets[bucket];

    while (idx != BLOCK_HASH_INDEX_NONE) {
        const SharedBlockHashEntry *entry = &store->entries[idx];
        if (entry->inUse && entry->keyHash == hash && 0 == strcmp(entry->key, key)) {
            if (bucketOut != NULL) {
                *bucketOut = bucket;
            }
            if (prevOut != NULL) {
                *prevOut = prev;
            }
            return idx;
        }
        prev = idx;
        idx = entry->next;
    }

    if (bucketOut != NULL) {
        *bucketOut = bucket;
    }
    if (prevOut != NULL) {
        *prevOut = BLOCK_HASH_INDEX_NONE;
    }

    return BLOCK_HASH_INDEX_NONE;
}

// Returns the first unused entry slot from the fixed entry array.
static int findFreeEntryIndex(const SharedBlockHashStore *store)
{
    for (int i = 0; i < BLOCK_HASH_MAX_CAPACITY; i++) {
        if (!store->entries[i].inUse) {
            return i;
        }
    }
    return BLOCK_HASH_INDEX_NONE;
}

// Unlinks and clears one entry from its bucket chain.
static bool removeEntryByIndex(SharedBlockHashStore *store, const int index)
{
    if (index < 0 || index >= BLOCK_HASH_MAX_CAPACITY || !store->entries[index].inUse) {
        return false;
    }

    const unsigned int hash = store->entries[index].keyHash;
    const int bucket = (int)(hash % BLOCK_HASH_BUCKET_COUNT);

    int prev = BLOCK_HASH_INDEX_NONE;
    int cur = store->buckets[bucket];

    while (cur != BLOCK_HASH_INDEX_NONE) {
        if (cur == index) {
            if (prev == BLOCK_HASH_INDEX_NONE) {
                store->buckets[bucket] = store->entries[cur].next;
            } else {
                store->entries[prev].next = store->entries[cur].next;
            }

            memset(&store->entries[cur], 0, sizeof(SharedBlockHashEntry));
            store->entries[cur].next = BLOCK_HASH_INDEX_NONE;
            if (store->entryCount > 0) {
                store->entryCount--;
            }
            return true;
        }

        prev = cur;
        cur = store->entries[cur].next;
    }

    return false;
}

bool blockHashSetUpStore(const server_rec *serverRec, const int maxEntries)
{
    apr_pool_t *processPool = serverRec->process->pool;

    int configuredMaxEntries = maxEntries;
    if (configuredMaxEntries <= 0) {
        configuredMaxEntries = 1;
    }
    if (configuredMaxEntries > BLOCK_HASH_MAX_CAPACITY) {
        configuredMaxEntries = BLOCK_HASH_MAX_CAPACITY;
    }

    // Remove stale OS objects from older runs before creating fresh shm/mutex objects.
    apr_shm_remove(BLOCK_HASH_SHM_PATH, processPool);
    apr_file_remove(BLOCK_HASH_MUTEX_PATH, processPool);

    apr_status_t status = apr_shm_create(
        &blockHash.shm,
        sizeof(SharedBlockHashStore),
        BLOCK_HASH_SHM_PATH,
        processPool
    );
    if (status != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, status, serverRec, "blockHashSetUpStore: apr_shm_create failed");
        return false;
    }

    status = apr_proc_mutex_create(&blockHash.mutex, BLOCK_HASH_MUTEX_PATH, APR_LOCK_DEFAULT, processPool);
    if (status != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, status, serverRec, "blockHashSetUpStore: apr_proc_mutex_create failed");
        return false;
    }

    SharedBlockHashStore *store = getSharedStore();
    if (NULL == store) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, serverRec, "blockHashSetUpStore: shared memory base address is NULL");
        return false;
    }

    // Initialize the in-shm store state.
    memset(store, 0, sizeof(SharedBlockHashStore));
    for (int i = 0; i < BLOCK_HASH_BUCKET_COUNT; i++) {
        store->buckets[i] = BLOCK_HASH_INDEX_NONE;
    }
    for (int i = 0; i < BLOCK_HASH_MAX_CAPACITY; i++) {
        store->entries[i].next = BLOCK_HASH_INDEX_NONE;
    }

    store->maxEntries = configuredMaxEntries;
    store->entryCount = 0;
    store->initialized = 1;

    blockHash.blockHash = (void *)store;
    blockHash.maxEntries = configuredMaxEntries;
    blockHash.serverPool = processPool;

    ap_log_error(
        APLOG_MARK,
        APLOG_NOTICE,
        0,
        serverRec,
        "blockHashSetUpStore: shared blockHash initialized adr=0x%lx maxEntries=%d",
        (unsigned long)(void *)blockHash.blockHash,
        configuredMaxEntries
    );

    return true;
}

bool blockHashRemoveOldestEntry()
{
    SharedBlockHashStore *store = getSharedStore();
    if (NULL == store || !store->initialized || store->entryCount == 0) {
        return false;
    }

    int oldestIndex = BLOCK_HASH_INDEX_NONE;
    int oldestTs = INT_MAX;

    for (int i = 0; i < BLOCK_HASH_MAX_CAPACITY; i++) {
        if (!store->entries[i].inUse) {
            continue;
        }
        if (store->entries[i].tsLastUse < oldestTs) {
            oldestTs = store->entries[i].tsLastUse;
            oldestIndex = i;
        }
    }

    if (oldestIndex == BLOCK_HASH_INDEX_NONE) {
        return false;
    }

    return removeEntryByIndex(store, oldestIndex);
}

BlockHashEntry *blockHashGetEntry(const char *key)
{
    if (NULL == key || NULL == blockHash.mutex) {
        return NULL;
    }

    SharedBlockHashStore *store = getSharedStore();
    if (NULL == store || !store->initialized) {
        return NULL;
    }

    apr_proc_mutex_lock(blockHash.mutex);

    const unsigned int hash = keyHashDjb2(key);
    const int idx = findEntryIndex(store, key, hash, NULL, NULL);
    if (idx == BLOCK_HASH_INDEX_NONE) {
        apr_proc_mutex_unlock(blockHash.mutex);
        return NULL;
    }

    // Return a thread-local snapshot so callers never hold pointers into mutable shm state.
    static __thread BlockHashEntry outEntry;
    outEntry.doBlock = store->entries[idx].doBlock != 0;
    outEntry.blockType = (enum EnumBlockType)store->entries[idx].blockType;
    outEntry.tsLastUse = store->entries[idx].tsLastUse;
    outEntry.cnt = store->entries[idx].cnt;
    outEntry.key = store->entries[idx].key;

    apr_proc_mutex_unlock(blockHash.mutex);
    return &outEntry;
}

int blockHashAddEntry(
    const char *key,
    const enum EnumBlockType blockType,
    const bool doBlock,
    apr_pool_t *parentPool)
{
    (void)parentPool;

    if (NULL == key || NULL == blockHash.mutex) {
        return 1;
    }

    SharedBlockHashStore *store = getSharedStore();
    if (NULL == store || !store->initialized) {
        return 1;
    }

    apr_proc_mutex_lock(blockHash.mutex);

    const unsigned int hash = keyHashDjb2(key);
    int bucket = 0;
    int prev = BLOCK_HASH_INDEX_NONE;
    int idx = findEntryIndex(store, key, hash, &bucket, &prev);

    if (idx != BLOCK_HASH_INDEX_NONE) {
        // Entry exists: refresh last-use timestamp and policy data.
        store->entries[idx].tsLastUse = (int)time(NULL);
        store->entries[idx].doBlock = doBlock ? 1U : 0U;
        store->entries[idx].blockType = (unsigned char)blockType;
        store->entries[idx].cnt++;
        apr_proc_mutex_unlock(blockHash.mutex);
        return 2;
    }

    // Evict least-recently-used entry when configured capacity is reached.
    if (store->entryCount >= store->maxEntries) {
        if (!blockHashRemoveOldestEntry()) {
            apr_proc_mutex_unlock(blockHash.mutex);
            return 3;
        }
    }

    idx = findFreeEntryIndex(store);
    if (idx == BLOCK_HASH_INDEX_NONE) {
        apr_proc_mutex_unlock(blockHash.mutex);
        return 3;
    }

    SharedBlockHashEntry *entry = &store->entries[idx];
    memset(entry, 0, sizeof(SharedBlockHashEntry));

    entry->inUse     = 1;
    entry->keyHash   = hash;
    entry->tsLastUse = (int)time(NULL);
    entry->doBlock   = doBlock ? 1U : 0U;
    entry->blockType = (unsigned char)blockType;
    entry->cnt       = 1;
    entry->next = store->buckets[bucket];

    strncpy(entry->key, key, BLOCK_HASH_KEY_MAX_LEN);
    entry->key[BLOCK_HASH_KEY_MAX_LEN] = '\0';

    store->buckets[bucket] = idx;
    store->entryCount++;

    apr_proc_mutex_unlock(blockHash.mutex);
    return 4;
}

int blockHashGetEntryCount()
{
    if (NULL == blockHash.mutex) {
        return -1;
    }

    SharedBlockHashStore *store = getSharedStore();
    if (NULL == store || !store->initialized) {
        return -1;
    }

    apr_proc_mutex_lock(blockHash.mutex);
    const int count = store->entryCount;
    apr_proc_mutex_unlock(blockHash.mutex);
    return count;
}

unsigned long blockHashGetSharedStoreSizeKb()
{
    return (unsigned long)sizeof(SharedBlockHashStore) / 1024;
}

static SharedBlockHashStore *gSortStore = NULL;

static int compareEntryIndexByTs(const void *a, const void *b)
{
    const int idxA = *(const int *)a;
    const int idxB = *(const int *)b;

    const int tsA = gSortStore->entries[idxA].tsLastUse;
    const int tsB = gSortStore->entries[idxB].tsLastUse;

    if (tsA < tsB) {
        return -1;
    }
    if (tsA > tsB) {
        return 1;
    }
    return 0;
}

void blockHashGetOldestAndNewestEntries(
    BlockHashEntry *oldest_entries[10],
    int *num_oldest,
    BlockHashEntry *newest_entries[10],
    int *num_newest)
{
    *num_oldest = 0;
    *num_newest = 0;

    if (NULL == blockHash.mutex) {
        return;
    }

    SharedBlockHashStore *store = getSharedStore();
    if (NULL == store || !store->initialized) {
        return;
    }

    apr_proc_mutex_lock(blockHash.mutex);

    if (store->entryCount <= 0) {
        apr_proc_mutex_unlock(blockHash.mutex);
        return;
    }

    int usedIdx[BLOCK_HASH_MAX_CAPACITY];
    int usedCount = 0;

    for (int i = 0; i < BLOCK_HASH_MAX_CAPACITY; i++) {
        if (store->entries[i].inUse) {
            usedIdx[usedCount++] = i;
        }
    }

    if (usedCount == 0) {
        apr_proc_mutex_unlock(blockHash.mutex);
        return;
    }

    gSortStore = store;
    qsort(usedIdx, (size_t)usedCount, sizeof(int), compareEntryIndexByTs);

    static __thread BlockHashEntry oldestSnap[10];
    static __thread BlockHashEntry newestSnap[10];

    *num_oldest = usedCount < 10 ? usedCount : 10;
    for (int i = 0; i < *num_oldest; i++) {
        const SharedBlockHashEntry *entry = &store->entries[usedIdx[i]];
        oldestSnap[i].doBlock = entry->doBlock != 0;
        oldestSnap[i].blockType = (enum EnumBlockType)entry->blockType;
        oldestSnap[i].tsLastUse = entry->tsLastUse;
        oldestSnap[i].cnt = entry->cnt;
        oldestSnap[i].key = entry->key;
        oldest_entries[i] = &oldestSnap[i];
    }

    *num_newest = usedCount < 10 ? usedCount : 10;
    for (int i = 0; i < *num_newest; i++) {
        const SharedBlockHashEntry *entry = &store->entries[usedIdx[usedCount - 1 - i]];
        newestSnap[i].doBlock = entry->doBlock != 0;
        newestSnap[i].blockType = (enum EnumBlockType)entry->blockType;
        newestSnap[i].tsLastUse = entry->tsLastUse;
        newestSnap[i].cnt = entry->cnt;
        newestSnap[i].key = entry->key;
        newest_entries[i] = &newestSnap[i];
    }

    apr_proc_mutex_unlock(blockHash.mutex);
}
