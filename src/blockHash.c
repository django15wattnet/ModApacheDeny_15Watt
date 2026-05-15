#include <apr_hash.h>
#include <apr_strings.h>
#include <apr_shm.h>
#include <apr_proc_mutex.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>
#include "blockHash.h"

#include <httpd.h>
#include <http_log.h>

const char* BlockTypeStrings[] = {
    "Not Found",
    "None",
    "IPv4",
    "IPv6",
    "CIDR IPv4",
    "CIDR IPv6",
    "User Agent"
};

// The hash
BlockHash blockHash;


/**
 * Initializes the global block hash store.
 *
 * Creates a new APR hash table using the process pool of the given server record,
 * sets the maximum number of entries, and creates a thread mutex to protect
 * concurrent access from multiple threads (required for MPM Event).
 *
 * This function must be called once during the server configuration phase
 * (ap_hook_post_config) before any calls to blockHashAddEntry or blockHashGetEntry.
 * Calling it again will replace the existing hash with a new empty one.
 *
 * @param serverRec   Pointer to the server_rec. Its process pool is used as the
 *                    parent pool for the hash, the mutex, and all entry sub-pools.
 * @param maxEntries  Maximum number of entries the hash may hold. When this limit
 *                    is reached, the oldest entry (by tsLastUse) is removed before
 *                    a new one is inserted.
 *
 * @return true   on success.
 * @return false  if the hash table or the mutex could not be created.
 */
bool blockHashSetUpStore(const server_rec *serverRec, const int maxEntries)
{
    apr_pool_t *processPool = serverRec->process->pool;

    // Create shared memory segment
    apr_status_t status = apr_shm_create(&blockHash.shm, sizeof(BlockHash), "/tmp/blockhash_shm", processPool);
    if (status != APR_SUCCESS) {
        ap_log_error(
            APLOG_MARK,
            APLOG_ERR,
            status,
            serverRec,
            "blockHashSetUpStore: apr_shm_create failed"
        );
        return false;
    }

    // Initialize process mutex for inter-process synchronization
    status = apr_proc_mutex_create(&blockHash.mutex, "/tmp/blockhash_mutex", APR_LOCK_DEFAULT, processPool);
    if (status != APR_SUCCESS) {
        ap_log_error(
            APLOG_MARK,
            APLOG_ERR,
            status,
            serverRec,
            "blockHashSetUpStore: apr_proc_mutex_create failed"
        );
        return false;
    }

    // Get base address of shared memory
    BlockHash *shared_hash = (BlockHash *)apr_shm_baseaddr_get(blockHash.shm);

    // Initialize shared hash structure
    shared_hash->blockHash = apr_hash_make(processPool);
    if (NULL == shared_hash->blockHash) {
        ap_log_error(
            APLOG_MARK,
            APLOG_ERR,
            0,
            serverRec,
            "blockHashSetUpStore: apr_hash_make failed"
        );
        return false;
    }

    ap_log_error(
        APLOG_MARK,
        APLOG_ERR,
        0,
        serverRec,
        "blockHashSetUpStore: blockHash created at adr = 0x%lx", (unsigned long)(void *)shared_hash->blockHash
    );

    shared_hash->maxEntries = maxEntries;
    shared_hash->serverPool = processPool;
    shared_hash->mutex = blockHash.mutex;
    shared_hash->shm = blockHash.shm;

    // Update global blockHash to point to shared memory structure
    blockHash.blockHash = shared_hash->blockHash;
    blockHash.maxEntries = maxEntries;
    blockHash.serverPool = processPool;

    return true;
}


/**
 * Adds a new entry to the hash or updates an existing one.
 *
 * Returns:
 *  - 1 = the hash is not initialized
 *  - 2 = existing entry updated
 *  - 3 = error while removing the oldest entry (when maxEntries is reached)
 *  - 4 = new entry successfully added to the hash
 *
 * @param key
 * @param blockType
 * @param doBlock
 * @param parentPool
 * @return
 */
int blockHashAddEntry(
    const char *key,
    const enum EnumBlockType blockType,
    const bool doBlock,
    apr_pool_t *parentPool)
{
    if (blockHash.blockHash == NULL) {
        return 1;
    }

    apr_proc_mutex_lock(blockHash.mutex);

    // Check if key is already in the block hash
    BlockHashEntry *entry = apr_hash_get(blockHash.blockHash, key, APR_HASH_KEY_STRING);
    if (entry != NULL) {
        entry->tsLastUse = time(NULL);
        apr_hash_set(blockHash.blockHash, key, APR_HASH_KEY_STRING, entry);
        apr_proc_mutex_unlock(blockHash.mutex);
        return 2;
    }

    if (apr_hash_count(blockHash.blockHash) >= blockHash.maxEntries) {
        if (!blockHashRemoveOldestEntry()) {
            apr_proc_mutex_unlock(blockHash.mutex);
            return 3;
        }
    }

    // Add new entry to the hash list — eigener Sub-Pool pro Eintrag
    apr_pool_t *entryPool;
    apr_pool_create(&entryPool, blockHash.serverPool);

    entry = apr_palloc(entryPool, sizeof(BlockHashEntry));

    entry->pool      = entryPool;
    entry->doBlock   = doBlock;
    entry->blockType = blockType;
    entry->tsLastUse = time(NULL);
    entry->key       = apr_pstrdup(entryPool, key);

    // Duplicate the key into the entryPool so it is not bound to the request pool
    const char *keyCopy = apr_pstrdup(entryPool, key);
    apr_hash_set(blockHash.blockHash, keyCopy, APR_HASH_KEY_STRING, entry);

    apr_proc_mutex_unlock(blockHash.mutex);
    return 4;
}


/**
 * Returns the BlockHashEntry for the key, or NULL if the key is not in the hash.
 *
 * @param key
 * @return
 */
BlockHashEntry *blockHashGetEntry(const char *key)
{
    if (blockHash.blockHash == NULL) {
        return NULL;
    }

    apr_proc_mutex_lock(blockHash.mutex);
    BlockHashEntry *entry = apr_hash_get(blockHash.blockHash, key, APR_HASH_KEY_STRING);
    apr_proc_mutex_unlock(blockHash.mutex);
    return entry;
}


/**
 * Returns the count of entries in the hash or -1 if the hash is not initialized.
 * @return int
 */
int blockHashGetEntryCount() {
    if (blockHash.blockHash == NULL) {
        return -1;
    }

    apr_proc_mutex_lock(blockHash.mutex);
    const int count = (int)apr_hash_count(blockHash.blockHash);
    apr_proc_mutex_unlock(blockHash.mutex);
    return count;
}


/**
 * Removes the oldest entry from the block hash based on the tsLastUse timestamp.
 *
 * Iterates over all entries in the hash and identifies the one with the smallest
 * tsLastUse value (i.e., the least recently used entry). That entry is then removed
 * from the hash and its associated memory pool is destroyed.
 *
 * This function is intended to be called internally by blockHashAddEntry when the
 * maximum number of entries has been reached. It must NOT be called while the mutex
 * is not already held by the caller — it does not acquire the mutex itself to avoid
 * a deadlock.
 *
 * @return true  if the oldest entry was successfully found and removed.
 * @return false if the hash is NULL, empty, or no entry could be identified.
 */
bool blockHashRemoveOldestEntry()
{
    if (blockHash.blockHash == NULL || apr_hash_count(blockHash.blockHash) == 0) {
        return false;
    }

    const void  *oldestKey    = NULL;
    apr_ssize_t  oldestKeyLen = 0;
    void        *oldestVal    = NULL;
    time_t       oldestTs     = (time_t)LONG_MAX;

    // Iterate over all entries
    for (apr_hash_index_t *hi = apr_hash_first(NULL, blockHash.blockHash); hi; hi = apr_hash_next(hi)) {
        const void  *key;
        apr_ssize_t  keyLen;
        void        *val;

        apr_hash_this(hi, &key, &keyLen, &val);
        BlockHashEntry *entry = (BlockHashEntry *)val;

        if (entry->tsLastUse < oldestTs) {
            oldestTs     = entry->tsLastUse;
            oldestKey    = key;
            oldestKeyLen = keyLen;
            oldestVal    = val;
        }
    }

    if (oldestKey == NULL) {
        return false;
    }

    /* Eintrag zuerst aus dem Hash entfernen, solange oldestKey noch gültig ist */
    apr_hash_set(blockHash.blockHash, oldestKey, oldestKeyLen, NULL);

    /* Danach Subpool des Eintrags zerstören — gibt den apr_palloc-Speicher frei */
    apr_pool_destroy(((BlockHashEntry *)oldestVal)->pool);

    return true;
}


void blockHashGetOldestAndNewestEntries(
    BlockHashEntry* oldest_entries[10],
    int* num_oldest,
    BlockHashEntry* newest_entries[10],
    int* num_newest)
{
    *num_oldest = 0;
    *num_newest = 0;

    if (blockHash.blockHash == NULL) {
        return;
    }

    apr_proc_mutex_lock(blockHash.mutex);

    const int count = apr_hash_count(blockHash.blockHash);
    if (count == 0) {
        apr_proc_mutex_unlock(blockHash.mutex);
        return;
    }

    BlockHashEntry** all_entries = apr_palloc(blockHash.serverPool, sizeof(BlockHashEntry*) * count);
    int i = 0;
    for (apr_hash_index_t* hi = apr_hash_first(NULL, blockHash.blockHash); hi; hi = apr_hash_next(hi)) {
        void* val;
        const void* key;
        apr_hash_this(hi, &key, NULL, &val);
        all_entries[i] = (BlockHashEntry*)val;
        all_entries[i]->key = (const char*)key;
        i++;
    }

    // Sort entries by tsLastUse
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (all_entries[j]->tsLastUse > all_entries[j + 1]->tsLastUse) {
                BlockHashEntry* temp = all_entries[j];
                all_entries[j] = all_entries[j + 1];
                all_entries[j + 1] = temp;
            }
        }
    }

    *num_oldest = count < 10 ? count : 10;
    for (int i = 0; i < *num_oldest; i++) {
        oldest_entries[i] = all_entries[i];
    }

    *num_newest = count < 10 ? count : 10;
    for (int i = 0; i < *num_newest; i++) {
        newest_entries[i] = all_entries[count - 1 - i];
    }

    apr_proc_mutex_unlock(blockHash.mutex);
}
