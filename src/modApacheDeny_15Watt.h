//
// Created by Thomas Siemion on 29.11.25.
//

#ifndef MODAPACHEDENY_15WATT_MODAPACHEDENY_15WATT_H
#define MODAPACHEDENY_15WATT_MODAPACHEDENY_15WATT_H

#include <stdio.h>
#include <stdlib.h>
#include "apr_hash.h"
#include "apr_tables.h"
#include "apr_hooks.h"
#include "apr_strings.h"
#include "ap_config.h"
#include "ap_provider.h"
#include "httpd.h"
#include "http_core.h"
#include "http_config.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_request.h"
#include "ap_config.h"
#include <mysql/mysql.h>
#include <arpa/inet.h>

#include "checkIpAddr.h"
#include "functionsString.h"

/* Configuration structure for the module */
typedef struct {
    const char *dbHost;
    const char *dbUser;
    const char *dbPwd;
    int         dbPort;
    const char *database;
    const char *tableAddresses;
    const char *tableUserAgents;
    const char *tableUserAgentsWl;
    int  allowEmptyUserAgent;
} ModuleConfig;

#define DoAllowEmptyUserAgent    1
#define DoNotAllowEmptyUserAgent 2

/* Structure to hold the user agents to block */
enum CompareType {
    CompareType_Contains   = 1,
    CompareType_Equals     = 2,
    CompareType_StartsWith = 3,
    CompareType_EndsWith   = 4
};

typedef struct {
    char *userAgent;
    enum CompareType compareType;
} UserAgentInfo;

// Structure to hold user agents to block or white list
typedef struct {
    int cntUserAgents;
    UserAgentInfo userAgents[];
} ModuleDataUserAgents;

/* Structure to hold IPv4 network to block information */
typedef struct {
    int cntNetInfoIpV4;
    NetInfoIpV4 *netInfoIpV4[];
} ModuleDataNetInfoIpV4;

/* Structure to hold IPv4 addresses to block */
typedef struct {
    int cntIpV4;
    char *ipV4[];
} ModuleDataIpV4;

/* Structure to hold IPv6 addresses to block */
typedef struct {
    int cntIpV6;
    char *ipV6[];
} ModuleDataIpV6;

/* Structure to hold IPv6 network to block */
typedef struct {
  int cntNetInfoIpV6;
  NetInfoIpV6 *netInfoIpV6[];
} ModuleDataNetInfoIpV6;

/* Structure to hold hostnames to block */
typedef struct {
    int cntHostnames;
    char *hostnames[];
} ModuleDataHostnames;

/* @todo Struktur zum speichern von IpV6-Netzwerken */


static void register_hooks(apr_pool_t *pool);
static int requestHandler(request_rec *r);
int handlerServerConfig(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s);

#include "shouldUserAgentBeBlocked.h"
#include "loadUserAgents.h"
#include "loadIpNetworks.h"

#endif //MODAPACHEDENY_15WATT_MODAPACHEDENY_15WATT_H