//
// Created by Thomas Siemion on 11.04.26.
//
#include <apr_hash.h>
#include <apr_strings.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>
#include <stdio.h>
#include "blockHash.h"

#include <httpd.h>
#include <http_log.h>


BlockHash blockHash;

/*
 * Die einzelnen Einträge im hash sind per apr_palloc alloziert.
    * apr_pool_t *entryPool;
    > apr_pool_create(&entryPool, parentPool);
    > BlockHashEntry *entry = apr_palloc(entryPool, sizeof(BlockHashEntry));
    > entry->pool = entryPool;
 */
bool blockHashSetUpStore(server_rec *serverRec, apr_pool_t *pool, const int maxEntries)
{
    blockHash.blockHash = apr_hash_make(pool);
    if (NULL == blockHash.blockHash) {
        ap_log_error(
            APLOG_MARK,
            APLOG_ERR,
            0,
            serverRec,
            "blockHashSetUpStore failed"
        );
        return false;
    }

    ap_log_error(
        APLOG_MARK,
        APLOG_ERR,
        0,
        serverRec,
        "blockHashSetUpStore adr = %p", blockHash.blockHash
    );

    blockHash.maxEntries = maxEntries;

    return true;
}


int blockHashAddEntry(
    const char *key,
    const enum EnumBlockType blockType,
    const bool doBlock,
    apr_pool_t *parentPool)
{
    if (blockHash.blockHash == NULL) {
        return 1;
    }

    // Check if key is already in the block hash
    BlockHashEntry *entry = apr_hash_get(blockHash.blockHash, key, APR_HASH_KEY_STRING);
    if (entry != NULL) {
        // Set entry's tsLastUser to now
        entry->tsLastUse = time(NULL);

        // save the updated entry to the block hash
        apr_hash_set(blockHash.blockHash, key, APR_HASH_KEY_STRING, entry);
        return 2;
    }

    if (apr_hash_count(blockHash.blockHash) >= blockHash.maxEntries) {
        // To many entries, remove the oldest
        if (!blockHashRemoveOldestEntry()) {
            return 3;
        }
    }

    // Add new entry to the hash list
    apr_pool_t *entryPool;
    apr_pool_create(&entryPool, parentPool);

    entry = apr_palloc(entryPool, sizeof(BlockHashEntry));

    entry->pool      = entryPool;
    entry->doBlock   = doBlock;
    entry->blockType = blockType;
    entry->tsLastUse = time(NULL);

    apr_hash_set(blockHash.blockHash, key, APR_HASH_KEY_STRING, entry);

    return 4;
}


/**
 * Returns the BlockHashEntry for the key key
 *
 * @param key
 * @return
 */
BlockHashEntry *blockHashGetEntry(const char *key)
{
    if (blockHash.blockHash == NULL) {
        return NULL;
    }

   return apr_hash_get(blockHash.blockHash, key, APR_HASH_KEY_STRING);
}


int blockHashGetEntryCount() {
    if (blockHash.blockHash == NULL) {
        return -1;
    }

    return apr_hash_count(blockHash.blockHash);
}


/*
    apr_pool_t *entryPool;
    > apr_pool_create(&entryPool, parentPool);
    > BlockHashEntry *entry = apr_palloc(entryPool, sizeof(BlockHashEntry));
    > entry->pool = entryPool;
 */
bool blockHashRemoveOldestEntry()
{
    if (blockHash.blockHash == NULL || apr_hash_count(blockHash.blockHash) == 0) {
        return false;
    }

    const void       *oldestKey    = NULL;
    apr_ssize_t       oldestKeyLen = 0;
    void             *oldestVal    = NULL;
    int               oldestTs     = INT_MAX;

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
