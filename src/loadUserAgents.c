//
// Created by Thomas Siemion on 13.12.25.
//

#include "loadUserAgents.h"

int loadUserAgents(
    const ModuleConfig *moduleConfig,
    ModuleDataUserAgents **data,
    apr_pool_t *pool,
    const server_rec *serverRec
) {
    printf("loadUserAgents\n");
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

    if (mysql_real_connect(conn, moduleConfig->dbHost, moduleConfig->dbUser,
                           moduleConfig->dbPwd, moduleConfig->database, 0, NULL, 0) == NULL) {
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
    snprintf(query, sizeof(query), "SELECT value FROM %s", moduleConfig->tableUserAgents);

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

    int cntUserAgents = (int)mysql_num_rows(res);
    *data = apr_pcalloc(pool, sizeof(ModuleDataUserAgents) + cntUserAgents * sizeof(const char *));
    (*data)->cntUserAgents = cntUserAgents;

    int index = 0;
    while ((row = mysql_fetch_row(res))) {
        (*data)->userAgents[index] = apr_pstrdup(pool, row[0]);
        index++;
    }

    mysql_free_result(res);
    mysql_close(conn);

    return 0;
}
