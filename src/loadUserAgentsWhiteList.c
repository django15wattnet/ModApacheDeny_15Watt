#include "loadUserAgentsWhiteList.h"

/**
 * Loads all white-listed user agents from the database and populates the
 * in-memory data structure.
 *
 * Connects to the MySQL database specified in @p moduleConfig, queries the
 * table defined by moduleConfig->tableUserAgentsWl, and stores each row value
 * together with its detected compare type in an APR-allocated array.
 *
 * The compare type is derived from optional '#' markers surrounding the value
 * (see detectCompareType()). The markers are stripped from the stored string
 * (see cutCompareTypeMarkers()).
 *
 * If a request's user agent matches an entry in this white-list, the request
 * is passed through without further checks, regardless of any block rules.
 *
 * All allocated memory is bound to @p pool and will be released when that pool
 * is destroyed.
 *
 * @param moduleConfig              Module configuration holding database
 *                                  credentials and the name of the white-list
 *                                  table.
 * @param dataUserAgentsWhiteList   Out-parameter. Receives a pointer to the
 *                                  allocated ModuleDataUserAgents structure.
 * @param pool                      APR pool used for all memory allocations.
 * @param serverRec                 Apache server record used for error logging.
 *
 * @return  0  on success.
 * @return -1  if mysql_init(), mysql_store_result(), or the SELECT query fails.
 * @return -2  if the database connection could not be established.
 */
int loadUserAgentsWhiteList(
    const ModuleConfig *moduleConfig,
    ModuleDataUserAgents **dataUserAgentsWhiteList,
    apr_pool_t *pool,
    const server_rec *serverRec
) {
    MYSQL_ROW row;
    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL) {
        ap_log_error(
            APLOG_MARK,
            APLOG_ERR,
            0,
            serverRec,
            "mysql_init() failed"
        );

        return -1;
    }

    if (mysql_real_connect(
            conn,
            moduleConfig->dbHost,
            moduleConfig->dbUser,
            moduleConfig->dbPwd,
            moduleConfig->database,
            moduleConfig->dbPort,
            NULL,
            0) == NULL) {
        ap_log_error(
            APLOG_MARK,
            APLOG_ERR,
            0,
            serverRec,
            "mysql_real_connect() failed"
        );
        mysql_close(conn);
        return -2;
    }

    char query[1024];
    snprintf(query, sizeof(query), "SELECT value FROM %s", moduleConfig->tableUserAgentsWl);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "SELECT query failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return -1;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (res == NULL) {
        fprintf(stderr, "mysql_store_result() failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return -1;
    }

    const int cntUserAgentsWl = (int)mysql_num_rows(res);

    // size of                ↓ Integer     ↓ + count strings * size of UserAgentInfo
    *dataUserAgentsWhiteList = apr_pcalloc(pool, sizeof(int) + cntUserAgentsWl * sizeof(UserAgentInfo));
    (*dataUserAgentsWhiteList)->cntUserAgents = cntUserAgentsWl;

    int index = 0;
    while ((row = mysql_fetch_row(res))) {
        const enum CompareType compareType = detectCompareType(row[0]);
        cutCompareTypeMarkers(row[0], compareType);

        (*dataUserAgentsWhiteList)->userAgents[index].compareType = compareType;
        (*dataUserAgentsWhiteList)->userAgents[index].userAgent   = apr_pstrdup(pool, row[0]);

        index++;
    }

    mysql_free_result(res);
    mysql_close(conn);

    return 0;
}