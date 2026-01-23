//
// Created by Thomas Siemion on 29.11.25.
//
#include "modApacheDeny_15Watt.h"

#include "loadUserAgentsWl.h"

/* ********************************** */
/* START config / directives handlers */
ModuleConfig moduleConfig;

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
    moduleConfig.dbPort = atoi(arg);
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

static const command_rec modApacheDeny_15Watt_directives[] =
{
    AP_INIT_TAKE1("modApacheDeny_15Watt_dbHost",              setConfigDbHost,              NULL, RSRC_CONF, "Database host"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_dbUser",              setConfigDbUser,              NULL, RSRC_CONF, "Database user"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_dbPwd",               setConfigDbPwd,               NULL, RSRC_CONF, "Database pasword"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_dbPort",              setConfigDbPort,              0   , RSRC_CONF, "Database port"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_database",            setConfigDatabase,            NULL, RSRC_CONF, "Name of the database holding the tables"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_tableAddresses",      setConfigTableAddresses,      NULL, RSRC_CONF, "Name of the database table holding the addresses to block"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_tableUserAgents",     setConfigTableUserAgents,     NULL, RSRC_CONF, "Name of the database table holding the user agents to block"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_tableUserAgentsWl",   setConfigTableUserAgentsWl,   NULL, RSRC_CONF, "Name of the database table holding the user agents to white list"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_allowEmptyUserAgent", setConfigAllowEmptyUserAgent, NULL, RSRC_CONF, "Are empty or missing User-Agent headers allowed (on/off)"),
    { NULL }
};
/* END config / directives handlers */
/* ******************************** */


/* ***************** */
/* START Module data */
ModuleDataUserAgents  *moduleDataUserAgents   = NULL;
ModuleDataUserAgents  *moduleDataUserAgentsWl = NULL;
ModuleDataHostnames   *moduleDataHostnames    = NULL;
ModuleDataIpV4        *moduleDataIpV4         = NULL;
ModuleDataIpV6        *moduleDataIpV6         = NULL;
ModuleDataNetInfoIpV4 *moduleDataNetInfoIpV4  = NULL;
ModuleDataNetInfoIpV6 *moduleDataNetInfoIpV6  = NULL;

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
 */
int handlerServerConfig(
    apr_pool_t *pconf,
    apr_pool_t *plog,
    apr_pool_t *ptemp,
    server_rec *s
)
{
    if (ap_state_query(AP_SQ_MAIN_STATE) == AP_SQ_MS_CREATE_PRE_CONFIG) {
        // Running syntax checks
        // Set marker for not set allowEmptyUserAgent
        // moduleConfig.allowEmptyUserAgent = -1;
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
        if (0 != loadUserAgentsWl(&moduleConfig, &moduleDataUserAgentsWl, pconf, s)) {
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

    return OK;
}


static void register_hooks(apr_pool_t *pool)
{
    /* Create a hook in the request handler, so we get called when a request arrives */
    static const char * const as_late_as_default[] = { "default-handler", NULL };
    ap_hook_handler(requestHandler, NULL, NULL, APR_HOOK_FIRST);

    /* Create a hook in the server configuration phase, so we can read our configuration */
    ap_hook_post_config(handlerServerConfig, NULL, NULL, APR_HOOK_MIDDLE);
}


static int requestHandler(request_rec *r)
{
    if (!r->handler) {
        return DECLINED;
    }

    const char *userAgent = apr_table_get(r->headers_in, "User-Agent");

    ap_log_rerror(
        APLOG_MARK,
        APLOG_INFO,
        0,
        r,
        "modApacheDeny_15Watt client info, user agent=%s ip=%s host=%s",
        userAgent,
        r->useragent_ip,
        r->useragent_host
    );

    if (
        (NULL == userAgent || 1 == strcmp(userAgent, ""))
        &&
        DoNotAllowEmptyUserAgent == moduleConfig.allowEmptyUserAgent
    ) {
        // Empty or not provided User-Agent is not allowed
        ap_log_rerror(
            APLOG_MARK,
            APLOG_INFO,
            0,
            r,
            "modApacheDeny_15Watt blocked client by empty user agent"
        );

        return HTTP_FORBIDDEN;
    }

    // Check whitelist user agenta
    if ((NULL != moduleDataUserAgentsWl) && (NULL != userAgent)) {
        // The request has a User-Agent, so check if it is in the block list
        if (true == shouldUserAgentBeBlocked(r, userAgent, moduleDataUserAgentsWl)) {
            ap_log_rerror(
                APLOG_MARK,
                APLOG_INFO,
                0,
                r,
                "modApacheDeny_15Watt white listed client by user agent=%s",
                userAgent
            );

            return DECLINED;
        }
    }

    if ((NULL != moduleDataUserAgents) && (NULL != userAgent)) {
        // The request has a User-Agent, so check if it is in the block list
        if (true == shouldUserAgentBeBlocked(r, userAgent, moduleDataUserAgents)) {
                ap_log_rerror(
                    APLOG_MARK,
                    APLOG_INFO,
                    0,
                    r,
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
        (NULL != r->useragent_ip)
        &&
        (addressType_IPv4 == detectAddressType(r->useragent_ip))
    ) {

        for (int i = 0; i < moduleDataIpV4->cntIpV4; i++) {
            if (0 == strcmp(r->useragent_ip, moduleDataIpV4->ipV4[i])) {
                ap_log_rerror(
                    APLOG_MARK,
                    APLOG_INFO,
                    0,
                    r,
                    "modApacheDeny_15Watt blocked client by ipV4 address=%s",
                    r->useragent_ip
                );

                return HTTP_FORBIDDEN;
            }
        }
    }

    // May block by clients ipV6
    if (
        (NULL != moduleDataIpV6)
        &&
        (NULL != r->useragent_ip)
        &&
        (addressType_IPv6 == detectAddressType(r->useragent_ip))) {

        for (int i = 0; i < moduleDataIpV6->cntIpV6; i++) {
            if (0 == strcmp(r->useragent_ip, moduleDataIpV6->ipV6[i])) {
                ap_log_rerror(
                    APLOG_MARK,
                    APLOG_INFO,
                    0,
                    r,
                    "modApacheDeny_15Watt blocked client by ipV6 address=%s",
                    r->useragent_ip
                );

                return HTTP_FORBIDDEN;
            }
        }
    }

    // May block clients by there hostname
    if (
        (NULL != moduleDataHostnames)
        &&
        (NULL != r->useragent_host)
    ) {
        for (int i = 0; i < moduleDataHostnames->cntHostnames; i++) {
            if (true == stringEndsWith(r->useragent_host, moduleDataHostnames->hostnames[i])) {
                ap_log_rerror(
                    APLOG_MARK,
                    APLOG_INFO,
                    0,
                    r,
                    "modApacheDeny_15Watt blocked client by hostname=%s",
                    r->useragent_host
                );

                return HTTP_FORBIDDEN;
            }
        }
    }


    // Block Ip by IpV6 CIDR
    if (
        (NULL != moduleDataNetInfoIpV6)
        &&
        (NULL != r->useragent_ip)
        &&
        (addressType_IPv6 == detectAddressType(r->useragent_ip))
    ) {
        for (int i = 0; i < moduleDataNetInfoIpV6->cntNetInfoIpV6; i++) {
            const NetInfoIpV6 *netInfoV6 = moduleDataNetInfoIpV6->netInfoIpV6[i];
            if (1 == isIpV6InNetwork(r->useragent_ip, netInfoV6)) {

                ap_log_rerror(
                    APLOG_MARK,
                    APLOG_INFO,
                    0,
                    r,
                    "modApacheDeny_15Watt blocked client ip=%s by ipv6 cidr=%s",
                    r->useragent_ip,
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
        (NULL != r->useragent_ip)
        &&
        (addressType_IPv4 == detectAddressType(r->useragent_ip))
    ) {
        struct in_addr ipAddr;
        if (1 == inet_pton(AF_INET, r->useragent_ip, &ipAddr)) {
            for (int i = 0; i < moduleDataNetInfoIpV4->cntNetInfoIpV4; i++) {
                const NetInfoIpV4 *netInfoV4 = moduleDataNetInfoIpV4->netInfoIpV4[i];

                if ((ipAddr.s_addr & netInfoV4->mask) == (netInfoV4->network & netInfoV4->mask)) {

                    ap_log_rerror(
                        APLOG_MARK,
                        APLOG_INFO,
                        0,
                        r,
                        "modApacheDeny_15Watt blocked client ip=%s by ipv4 cidr=%s",
                        r->useragent_ip,
                        netInfoV4->cidr
                    );

                    return HTTP_FORBIDDEN;
                }
            }
        }
    }

    // DECLINED means "not handled", so other modules can try to handle the request
    return DECLINED;
}
