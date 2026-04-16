//
// Created by Thomas Siemion on 21.01.26.
//

#include "loadUserAgentsWhiteList.h"

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