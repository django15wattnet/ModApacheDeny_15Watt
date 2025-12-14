//
// Created by Thomas Siemion on 29.11.25.
//
#include "modApacheDeny_15Watt.h"
#include "loadUserAgents.h"
#include "checkIpAddr.h"

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
    AP_INIT_TAKE1("modApacheDeny_15Watt_database",            setConfigDatabase,            NULL, RSRC_CONF, "Name of the database holding the tables"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_tableAddresses",      setConfigTableAddresses,      NULL, RSRC_CONF, "Name of the database table holding the addresses to block"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_tableUserAgents",     setConfigTableUserAgents,     NULL, RSRC_CONF, "Name of the database table holding the user agents to block"),
    AP_INIT_TAKE1("modApacheDeny_15Watt_allowEmptyUserAgent", setConfigAllowEmptyUserAgent, NULL, RSRC_CONF, "Are empty or missing User-Agent headers allowed (on/off)"),
    { NULL }
};
/* END config / directives handlers */
/* ******************************** */


/* ***************** */
/* START Module data */
ModuleDataUserAgents *moduleDataUserAgents = NULL;

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
    if (NULL == moduleConfig.database) {
        moduleConfig.database = "test";
    }
    if (NULL == moduleConfig.tableAddresses) {
        moduleConfig.tableAddresses = "block_ip_address";
    }
    if (NULL == moduleConfig.tableUserAgents) {
        moduleConfig.tableUserAgents = "block_user_agent";
    }

    printf("moduleConfig.allowEmptyUserAgent = %d\n", moduleConfig.allowEmptyUserAgent);

    if (0 == moduleConfig.allowEmptyUserAgent) {
        moduleConfig.allowEmptyUserAgent = 1;       // Default: allow empty User-Agent
    }

    // Build the datastructures for user agents to block
    if (NULL == moduleDataUserAgents) {
        if (0 != loadUserAgents(&moduleConfig, &moduleDataUserAgents, pconf, s)) {
            ap_log_error(
                APLOG_MARK,
                APLOG_ERR,
                0,
                s,
                "Failed to load user agents to block from database"
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
    if (NULL == moduleDataUserAgents) {
        return DECLINED;
    }

    if (!r->handler) {
        return DECLINED;
    }

    const char *user_agent = apr_table_get(r->headers_in, "User-Agent");

    if ((NULL == user_agent || 1 == strcmp(user_agent, "")) && 2 == moduleConfig.allowEmptyUserAgent) {
        // Empty or not provided User-Agent is not allowed
        return HTTP_FORBIDDEN;
    }

    if (NULL != user_agent) {
        for (int i = 0; i < moduleDataUserAgents->cntUserAgents; i++) {

            if (NULL != strstr(user_agent, moduleDataUserAgents->userAgents[i])) {
                ap_rprintf(r, "Blocked User Agent: %s matched %s<br>\n", user_agent, moduleDataUserAgents->userAgents[i]);
                return HTTP_FORBIDDEN;
            }
        }
    }

    // DECLINED means "not handled", so other modules can try to handle the request
    return DECLINED;



    /* Now that we are handling this request, we'll write out "Hello, world!" to the client.
     * To do so, we must first set the appropriate content type, followed by our output.
     */
    /*
    ap_set_content_type(r, "text/html");
    ap_rprintf(r, "Hello, world!\n<br>");

    const char *user_agent = apr_table_get(r->headers_in, "User-Agent");
    const char *client_ip = r->connection->client_ip;

    if (NULL == user_agent) {
        user_agent = "kein UserAgent";
    }

    ap_rprintf(r, "%s\n", user_agent);
    ap_rprintf(r, "\n<br>%s\n<br>", client_ip ? client_ip : "unbekannt");

    int addrType = detectAddressType(client_ip ? client_ip : "");
    ap_rprintf(r, "Address Type: %d\n<br>", addrType);

    // printf("Msg = %s\n", r->pool);


    ap_rprintf(r, "Loaded %d user agents to block\n<br>", moduleDataUserAgents->cntUserAgents);



    return OK;



    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    // MySQL-Verbindungsdetails
    const char *server = "192.168.64.1";
    const char *user = "root";
    const char *password = "monster1"; // Passwort ändern
    const char *database = "thomas.siemion.photography";

    // MySQL initialisieren
    conn = mysql_init(NULL);
    if (!conn) {
        ap_rputs("mysql_init() fehlgeschlagen\n", r);
        return OK; //HTTP_INTERNAL_SERVER_ERROR;
    }
    // Verbindung herstellen
    if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0)) {
        ap_rprintf(r, "Fehler bei Verbindung: %s\n", mysql_error(conn));
        mysql_close(conn);
        return OK; //HTTP_INTERNAL_SERVER_ERROR;
    }
    */

    // SQL-Query ausführen
    /*
    *
    SELECT
        COUNT(id) AS cnt
    FROM
        block_user_agent_site_data
    WHERE
         'XYT Custom-AsyncHttpClient ABC' LIKE concat('%', `value`, '%')
     */
    /*
    if (mysql_query(conn, "SELECT datum, title FROM news_site_data")) {
        ap_rprintf(r, "Query-Fehler: %s\n", mysql_error(conn));
        mysql_close(conn);
        return OK; //HTTP_INTERNAL_SERVER_ERROR;
    }

    // Ergebnis holen
    res = mysql_store_result(conn);
    if (!res) {
        ap_rprintf(r, "Ergebnisfehler: %s\n", mysql_error(conn));
        mysql_close(conn);
        return OK; //HTTP_INTERNAL_SERVER_ERROR;
    }

    // Ergebnis ausgeben
    int num_fields = mysql_num_fields(res);
    while ((row = mysql_fetch_row(res))) {
        for (int i = 0; i < num_fields; i++) {
            ap_rprintf(r,"%s\t", row[i] ? row[i] : "NULL");
        }
        int l = strlen(row[1]);
        ap_rprintf(r, "\tlen = %d<br>\n", l);
    }

    // Speicher freigeben
    mysql_free_result(res);
    mysql_close(conn);
*/
    /* Lastly, we must tell the server that we took care of this request and everything went fine.
     * We do so by simply returning the value OK to the server.
     */
    return OK;

}
