//
// Created by Thomas Siemion on 13.12.25.
//

#include "loadUserAgents.h"

int loadUserAgents(
    const ModuleConfig *moduleConfig,
    ModuleDataUserAgents **dataUserAgents,
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

    const int cntUserAgents = (int)mysql_num_rows(res);

    // size of                ↓ Integer     ↓ + count strings * size of UserAgentInfo
    *dataUserAgents = apr_pcalloc(pool, sizeof(int) + cntUserAgents * sizeof(UserAgentInfo));
    (*dataUserAgents)->cntUserAgents = cntUserAgents;

    int index = 0;
    while ((row = mysql_fetch_row(res))) {
        const enum CompareType compareType = detectCompareType(row[0]);
        cutCompareTypeMarkers(row[0], compareType);

        (*dataUserAgents)->userAgents[index].compareType = compareType;
        (*dataUserAgents)->userAgents[index].userAgent   = apr_pstrdup(pool, row[0]);

        index++;
    }

    mysql_free_result(res);
    mysql_close(conn);

    return 0;
}


enum CompareType detectCompareType(const char *userAgentStr)
{
    const int start = stringStartsWith(userAgentStr, "#");
    const int end   = stringEndsWith(userAgentStr, "#");

    if (1 == start && 1 == end) {
        return CompareType_Equals;
    }

    if (1 == start && 0 == end) {
        return CompareType_StartsWith;
    }

    if (0 == start && 1 == end) {
        return CompareType_EndsWith;
    }

    return CompareType_Contains;
}


void cutCompareTypeMarkers(char *userAgentStr, const enum CompareType compareType)
{
    switch (compareType) {
        case CompareType_Equals:
            // Remove both '#' characters
            stringDeleteCharRight(userAgentStr);
            stringDeleteCharLeft(userAgentStr);
            break;

        case CompareType_StartsWith:
            // Remove starting '#' character
            stringDeleteCharLeft(userAgentStr);
            break;

        case CompareType_EndsWith:
            // Remove ending '#' character
            stringDeleteCharRight(userAgentStr);
            break;

        case CompareType_Contains:
        default:
            // No markers to remove
            break;
    }
}
