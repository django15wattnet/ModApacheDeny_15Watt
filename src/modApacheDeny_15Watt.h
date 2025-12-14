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

/* Configuration structure for the module */
typedef struct {
    const char *dbHost;
    const char *dbUser;
    const char *dbPwd;
    const char *database;
    const char *tableAddresses;
    const char *tableUserAgents;
    int  allowEmptyUserAgent;
} ModuleConfig;

/* Structure to hold the user agents to block */
typedef struct {
    int cntUserAgents;
    char *userAgents[];
} ModuleDataUserAgents;


static void register_hooks(apr_pool_t *pool);
static int requestHandler(request_rec *r);
int handlerServerConfig(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s);

#endif //MODAPACHEDENY_15WATT_MODAPACHEDENY_15WATT_H