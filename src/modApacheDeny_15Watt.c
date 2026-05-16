#include "modApacheDeny_15Watt.h"
#include "status.h"

#include "blockHash.h"
#include "loadUserAgentsWhiteList.h"
#include <errno.h>
#include <limits.h>

static void register_hooks(apr_pool_t *pool);
static int requestHandler(request_rec *requestRec);

/* ********************************** */
/* START config / directives handlers */
ModuleConfig moduleConfig;

static int parseIntStrict(const char *arg, int minVal, int maxVal, int *outVal)
{
    if (NULL == arg || NULL == outVal) {
        return 0;
    }

    errno = 0;
    char *endPtr = NULL;
    long parsed = strtol(arg, &endPtr, 10);

    if (errno != 0 || endPtr == arg || *endPtr != '\0') {
        return 0;
    }

    if (parsed < (long)minVal || parsed > (long)maxVal) {
        return 0;
    }

    *outVal = (int)parsed;
    return 1;
}

const char *setConfigDbHost(cmd_parms *cmd, void *cfg, const char *arg)
{
    moduleConfig.dbHost = arg;
    return NULL;
}

const char *setConfigDbUser(cmd_parms *cmd, void *cfg, const char *arg)
{
    moduleConfig.dbUser = arg;
    return NULL;
}

const char *setConfigDbPwd(cmd_parms *cmd, void *cfg, const char *arg)
{
    moduleConfig.dbPwd = arg;
    return NULL;
}

const char *setConfigDbPort(cmd_parms *cmd, void *cfg, const char *arg)
{
    int parsedPort = 0;
    if (!parseIntStrict(arg, 1, 65535, &parsedPort)) {
        return "modApacheDeny_15Watt_dbPort must be an integer between 1 and 65535";
    }

    moduleConfig.dbPort = parsedPort;
    return NULL;
}

const char *setConfigDatabase(cmd_parms *cmd, void *cfg, const char *arg)
{
    moduleConfig.database = arg;
    return NULL;
}

const char *setConfigTableAddresses(cmd_parms *cmd, void *cfg, const char *arg)
{
    moduleConfig.tableAddresses = arg;
    return NULL;
}

const char *setConfigTableUserAgents(cmd_parms *cmd, void *cfg, const char *arg)
{
    moduleConfig.tableUserAgents = arg;
    return NULL;
}

const char *setConfigTableUserAgentsWl(cmd_parms *cmd, void *cfg, const char *arg)
{
    moduleConfig.tableUserAgentsWl = arg;
    return NULL;
}

const char *setConfigAllowEmptyUserAgent(cmd_parms *cmd, void *cfg, const char *arg)
{
    if (0 == strcasecmp(arg, "on") || 0 == strcasecmp(arg, "true") || 0 == strcasecmp(arg, "1")) {
        moduleConfig.allowEmptyUserAgent = 1;   // Allow empty User-Agent
    } else {
        moduleConfig.allowEmptyUserAgent = 2;   // Do not allow empty User-Agent
    }
    return NULL;
}

const char *setConfigAllowedHash(cmd_parms *cmd, void *cfg, const char *arg)
{
    if (0 == strcasecmp(arg, "on") || 0 == strcasecmp(arg, "true") || 0 == strcasecmp(arg, "1")) {
        moduleConfig.useAllowedHash = UseAllowedHash;
    } else {
        moduleConfig.useAllowedHash = DoNotUseAllowedHash;
    }
    return NULL;
}

const char *setConfigMaxAllowedHashEntries(cmd_parms *cmd, void *cfg, const char *arg)
{
    int val = 0;
    if (!parseIntStrict(arg, 1, INT_MAX, &val)) {
        return "modApacheDeny_15Watt_maxAllowedHashEntries must be a positive integer";
    }

    moduleConfig.maxAllowedHashEntries = val;
    return NULL;
}

static const command_rec modApacheDeny_15Watt_directives[] =
{
    AP_INIT_TAKE1("modApacheDeny_15Watt_dbHost",                  setConfigDbHost,                  NULL, RSRC_CONF, "Database host"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_dbUser",                  setConfigDbUser,                  NULL, RSRC_CONF, "Database user"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_dbPwd",                   setConfigDbPwd,                   NULL, RSRC_CONF, "Database pasword"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_dbPort",                  setConfigDbPort,                  0   , RSRC_CONF, "Database port"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_database",                setConfigDatabase,                NULL, RSRC_CONF, "Name of the database holding the tables"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_tableAddresses",          setConfigTableAddresses,          NULL, RSRC_CONF, "Name of the database table holding the addresses to block"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_tableUserAgents",         setConfigTableUserAgents,         NULL, RSRC_CONF, "Name of the database table holding the user agents to block"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_tableUserAgentsWl",       setConfigTableUserAgentsWl,       NULL, RSRC_CONF, "Name of the database table holding the user agents to white list"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_allowEmptyUserAgent",     setConfigAllowEmptyUserAgent,     NULL, RSRC_CONF, "Are empty or missing User-Agent headers allowed (on/off)"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_useAllowedHash",          setConfigAllowedHash,             NULL, RSRC_CONF, "Whether to use the allowed hash to store allowed entries (on/off)"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_maxAllowedHashEntries",   setConfigMaxAllowedHashEntries,   NULL, RSRC_CONF, "Maximum number of entries in the allowed/block hash"),
    { NULL }
};
/* END config / directives handlers */
/* ******************************** */


/* ***************** */
/* START Module data */
static int blockHashInitialized = 0;  /* Flag to ensure blockHash is initialized only once */

ModuleDataUserAgents  *moduleDataUserAgents          = NULL;
ModuleDataUserAgents  *moduleDataUserAgentsWhiteList = NULL;
ModuleDataHostnames   *moduleDataHostnames           = NULL;
ModuleDataIpV4        *moduleDataIpV4                = NULL;
ModuleDataIpV6        *moduleDataIpV6                = NULL;
ModuleDataNetInfoIpV4 *moduleDataNetInfoIpV4         = NULL;
ModuleDataNetInfoIpV6 *moduleDataNetInfoIpV6         = NULL;

/* END Module data   */
/* ***************** */


module AP_MODULE_DECLARE_DATA modApacheDeny_15Watt_module =
{
    STANDARD20_MODULE_STUFF,
    NULL,
    NULL,
    NULL,
    NULL,
    modApacheDeny_15Watt_directives,
    register_hooks   /* Our hook registering function */
};


/**
 * The configuration is read in.
 * Sets default values if no configuration was provided.
 * Initializes shared memory once in the parent process.
 */
int serverConfigHandler(
    apr_pool_t *pconf,
    apr_pool_t *plog,
    apr_pool_t *ptemp,
    server_rec *s
)
{
    if (ap_state_query(AP_SQ_MAIN_STATE) == AP_SQ_MS_CREATE_PRE_CONFIG) {
        // Running syntax checks
        return OK;
    }

    if (s->is_virtual) {
        return OK;
    }

    // If no configuration was provided, set default values
    if (NULL == moduleConfig.dbHost) {
        moduleConfig.dbHost = "localhost";
    }
    if (NULL == moduleConfig.dbUser) {
        moduleConfig.dbUser = "root";
    }
    if (NULL == moduleConfig.dbPwd) {
        moduleConfig.dbPwd = "";
    }
    if (0 == moduleConfig.dbPort) {
        moduleConfig.dbPort = 3306;
    }
    if (NULL == moduleConfig.database) {
        moduleConfig.database = "test";
    }
    if (NULL == moduleConfig.tableAddresses) {
        moduleConfig.tableAddresses = "block_ip_address";
    }
    if (NULL == moduleConfig.tableUserAgents) {
        moduleConfig.tableUserAgents = "block_user_agent";
    }
    if (NULL == moduleConfig.tableUserAgentsWl) {
        moduleConfig.tableUserAgentsWl = "block_user_agent_white_list";
    }
    if (0 == moduleConfig.allowEmptyUserAgent) {
        moduleConfig.allowEmptyUserAgent = 1;       // Default: allow empty User-Agent
    }
    if (0 == moduleConfig.useAllowedHash) {
        moduleConfig.useAllowedHash = UseAllowedHash; // Default: use allowed hash
    }
    if (0 == moduleConfig.maxAllowedHashEntries) {
        moduleConfig.maxAllowedHashEntries = DefaultMaxAllowedHashEntries;
    }

    if (NULL == moduleDataUserAgents) {
        // Build the datastructure for user agents to block
        if (0 != loadUserAgents(&moduleConfig, &moduleDataUserAgents, pconf, s)) {
            ap_log_error(
                APLOG_MARK,
                APLOG_ERR,
                0,
                s,
                "modApacheDeny_15Watt Failed to load user agents to block from database"
            );
        }

        // Build the datastructure for user agents to white list
        if (0 != loadUserAgentsWhiteList(&moduleConfig, &moduleDataUserAgentsWhiteList, pconf, s)) {
            ap_log_error(
                APLOG_MARK,
                APLOG_ERR,
                0,
                s,
                "modApacheDeny_15Watt Failed to load user agents to white list from database"
            );
        }

        // Build the datastructures for IP addresses, networks and hostnames to block
        if (0 != loadIpNetworks(
            &moduleConfig,
            &moduleDataHostnames,
            &moduleDataIpV4,
            &moduleDataIpV6,
            &moduleDataNetInfoIpV4,
            &moduleDataNetInfoIpV6,
            pconf,
            s
        )) {
            ap_log_error(
                APLOG_MARK,
                APLOG_ERR,
                0,
                s,
                "modApacheDeny_15Watt Failed to load IP addresses, networks and hostnames to block from database"
            );
        }
    }

    // Initialize shared memory once in the parent process
    // This happens after configuration but before worker processes are forked
    if (blockHashInitialized == 0) {
        ap_log_error(
            APLOG_MARK,
            APLOG_NOTICE,
            0,
            s,
            "modApacheDeny_15Watt: Initializing shared memory for blockHash"
        );

        blockHashSetUpStore(s, moduleConfig.maxAllowedHashEntries);
        blockHashInitialized = 1;
    }

    return OK;
}


static void register_hooks(apr_pool_t *pool)
{
    /* Create a hook in the request handler, so we get called when a request arrives */
    ap_hook_handler(requestHandler, NULL, NULL, APR_HOOK_FIRST);
    ap_hook_handler(statusHandler, NULL, NULL, APR_HOOK_MIDDLE);


    /* Create a hook in the server configuration phase, so we can read our configuration */
    ap_hook_post_config(serverConfigHandler, NULL, NULL, APR_HOOK_MIDDLE);
}


static int requestHandler(request_rec *requestRec)
{
    if (!requestRec->handler) {
        return DECLINED;
    }

    const char *userAgent = apr_table_get(requestRec->headers_in, "User-Agent");
    char userAgentKey[256];

    ap_log_rerror(
        APLOG_MARK,
        APLOG_INFO,
        0,
        requestRec,
        "modApacheDeny_15Watt client info, user agent=%s ip=%s host=%s",
        userAgent,
        requestRec->useragent_ip,
        requestRec->useragent_host
    );

    if (
        (NULL == userAgent || 0 == strcmp(userAgent, ""))
        &&
        DoNotAllowEmptyUserAgent == moduleConfig.allowEmptyUserAgent
    ) {
        // Empty or not provided User-Agent is not allowed
        ap_log_rerror(
            APLOG_MARK,
            APLOG_INFO,
            0,
            requestRec,
            "modApacheDeny_15Watt blocked client by empty user agent"
        );

        return HTTP_FORBIDDEN;
    }

    // Check whitelist user agent
    if ((NULL != moduleDataUserAgentsWhiteList) && (NULL != userAgent)) {
        // The request has a User-Agent, so check if it is in the white list
        if (true == shouldUserAgentBeBlocked(requestRec, userAgent, moduleDataUserAgentsWhiteList)) {
            ap_log_rerror(
                APLOG_MARK,
                APLOG_INFO,
                0,
                requestRec,
                "modApacheDeny_15Watt white listed client by user agent=%s",
                userAgent
            );

            return DECLINED;
        }
    }

    if (UseAllowedHash == moduleConfig.useAllowedHash) {

        if (NULL == userAgent) {
            userAgent = "---";
        }
        snprintf(userAgentKey, sizeof(userAgentKey), "%s|%s", userAgent, requestRec->useragent_ip);

        const BlockHashEntry *entry = blockHashGetEntry(userAgentKey);
        if (NULL != entry) {
            ap_log_rerror(
                APLOG_MARK,
                APLOG_INFO,
                0,
                requestRec,
                "modApacheDeny_15Watt allowed client by user agent=%s and ip=%s (cached)",
                userAgent,
                requestRec->useragent_ip
            );

            // Update entry->tsLastUse
            blockHashAddEntry(userAgentKey, entry->blockType, entry->doBlock, requestRec->pool);

            return DECLINED;
        }
    }

    if ((NULL != moduleDataUserAgents) && (NULL != userAgent)) {
        // The request has a User-Agent, so check if it is in the block list
        if (true == shouldUserAgentBeBlocked(requestRec, userAgent, moduleDataUserAgents)) {
                ap_log_rerror(
                    APLOG_MARK,
                    APLOG_INFO,
                    0,
                    requestRec,
                    "modApacheDeny_15Watt blocked client by user agent=%s",
                    userAgent
                );

            return HTTP_FORBIDDEN;
        }
    }

    // The user agent ist not blocked, so check the IP address / hostname

    // May block clients by there IpV4
    if (
        (NULL != moduleDataIpV4)
        &&
        (NULL != requestRec->useragent_ip)
        &&
        (addressType_IPv4 == detectAddressType(requestRec->useragent_ip))
    ) {

        for (int i = 0; i < moduleDataIpV4->cntIpV4; i++) {
            if (0 == strcmp(requestRec->useragent_ip, moduleDataIpV4->ipV4[i])) {
                ap_log_rerror(
                    APLOG_MARK,
                    APLOG_INFO,
                    0,
                    requestRec,
                    "modApacheDeny_15Watt blocked client by ipV4 address=%s",
                    requestRec->useragent_ip
                );

                return HTTP_FORBIDDEN;
            }
        }
    }

    // May block by clients ipV6
    if (
        (NULL != moduleDataIpV6)
        &&
        (NULL != requestRec->useragent_ip)
        &&
        (addressType_IPv6 == detectAddressType(requestRec->useragent_ip))) {

        for (int i = 0; i < moduleDataIpV6->cntIpV6; i++) {
            if (0 == strcmp(requestRec->useragent_ip, moduleDataIpV6->ipV6[i])) {
                ap_log_rerror(
                    APLOG_MARK,
                    APLOG_INFO,
                    0,
                    requestRec,
                    "modApacheDeny_15Watt blocked client by ipV6 address=%s",
                    requestRec->useragent_ip
                );

                return HTTP_FORBIDDEN;
            }
        }
    }

    // May block clients by there hostname
    if (
        (NULL != moduleDataHostnames)
        &&
        (NULL != requestRec->useragent_host)
    ) {
        for (int i = 0; i < moduleDataHostnames->cntHostnames; i++) {
            if (true == stringEndsWith(requestRec->useragent_host, moduleDataHostnames->hostnames[i])) {
                ap_log_rerror(
                    APLOG_MARK,
                    APLOG_INFO,
                    0,
                    requestRec,
                    "modApacheDeny_15Watt blocked client by hostname=%s",
                    requestRec->useragent_host
                );

                return HTTP_FORBIDDEN;
            }
        }
    }


    // Block Ip by IpV6 CIDR
    if (
        (NULL != moduleDataNetInfoIpV6)
        &&
        (NULL != requestRec->useragent_ip)
        &&
        (addressType_IPv6 == detectAddressType(requestRec->useragent_ip))
    ) {
        for (int i = 0; i < moduleDataNetInfoIpV6->cntNetInfoIpV6; i++) {
            const NetInfoIpV6 *netInfoV6 = moduleDataNetInfoIpV6->netInfoIpV6[i];
            if (1 == isIpV6InNetwork(requestRec->useragent_ip, netInfoV6)) {

                ap_log_rerror(
                    APLOG_MARK,
                    APLOG_INFO,
                    0,
                    requestRec,
                    "modApacheDeny_15Watt blocked client ip=%s by ipv6 cidr=%s",
                    requestRec->useragent_ip,
                    netInfoV6->cidr
                );

                return HTTP_FORBIDDEN;
            }
        }
    }


    // Block Ip by IpV4 CIDR
    if (
        (NULL != moduleDataNetInfoIpV4)
        &&
        (NULL != requestRec->useragent_ip)
        &&
        (addressType_IPv4 == detectAddressType(requestRec->useragent_ip))
    ) {
        struct in_addr ipAddr;
        if (1 == inet_pton(AF_INET, requestRec->useragent_ip, &ipAddr)) {
            for (int i = 0; i < moduleDataNetInfoIpV4->cntNetInfoIpV4; i++) {
                const NetInfoIpV4 *netInfoV4 = moduleDataNetInfoIpV4->netInfoIpV4[i];

                if ((ipAddr.s_addr & netInfoV4->mask) == (netInfoV4->network & netInfoV4->mask)) {

                    ap_log_rerror(
                        APLOG_MARK,
                        APLOG_INFO,
                        0,
                        requestRec,
                        "modApacheDeny_15Watt blocked client ip=%s by ipv4 cidr=%s",
                        requestRec->useragent_ip,
                        netInfoV4->cidr
                    );

                    return HTTP_FORBIDDEN;
                }
            }
        }
    }

    // The request is allowed
    if (UseAllowedHash == moduleConfig.useAllowedHash) {
        const int res = blockHashAddEntry(
            userAgentKey,
            blockTypeNone,
            false,
            requestRec->server->process->pool
        );

        const char *resMsg;
        switch (res) {
            case 1:  resMsg = "hash not initialized";
                break;

            case 2:  resMsg = "existing entry updated";
                break;

            case 3:  resMsg = "failed to remove oldest entry";
                break;

            case 4:  resMsg = "new entry added";
                break;

            default: resMsg = "unknown result";
                break;
        }

        ap_log_rerror(
            APLOG_MARK,
            APLOG_INFO,
            0,
            requestRec,
            "modApacheDeny_15Watt blockHashAddEntry: %s (key=%s)",
            resMsg,
            userAgentKey
        );
    }

    // DECLINED means "not handled", so other modules can try to handle the request
    return DECLINED;
}
