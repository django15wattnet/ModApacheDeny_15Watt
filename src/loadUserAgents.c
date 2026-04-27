#include "loadUserAgents.h"

/**
 * Loads all user agents to block from the database and populates the in-memory
 * data structure.
 *
 * Connects to the MySQL database specified in @p moduleConfig, queries the
 * table defined by moduleConfig->tableUserAgents, and stores each row value
 * together with its detected compare type in an APR-allocated array.
 *
 * The compare type is derived from optional '#' markers surrounding the value
 * (see detectCompareType()). The markers are stripped from the stored string
 * (see cutCompareTypeMarkers()).
 *
 * All allocated memory is bound to @p pool and will be released when that pool
 * is destroyed.
 *
 * @param moduleConfig    Module configuration holding database credentials and
 *                        the name of the user-agents table.
 * @param dataUserAgents  Out-parameter. Receives a pointer to the allocated
 *                        ModuleDataUserAgents structure.
 * @param pool            APR pool used for all memory allocations.
 * @param serverRec       Apache server record used for error logging.
 *
 * @return  0  on success.
 * @return -1  if mysql_init(), mysql_store_result(), or the SELECT query fails.
 * @return -2  if the database connection could not be established.
 */
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


/**
 * Determines the compare type for a user-agent string based on '#' markers.
 *
 * The following marker conventions are supported:
 *  - No markers  → CompareType_Contains   (e.g. "bot")
 *  - Leading '#' → CompareType_StartsWith (e.g. "#Mozilla")
 *  - Trailing '#'→ CompareType_EndsWith   (e.g. "Safari#")
 *  - Both '#'    → CompareType_Equals     (e.g. "#Googlebot#")
 *
 * @param userAgentStr  The raw user-agent string as read from the database.
 *                      Must be a valid, null-terminated C string.
 *
 * @return The detected CompareType value.
 */
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


/**
 * Removes the '#' marker characters from a user-agent string in-place,
 * according to the given compare type.
 *
 *  - CompareType_Equals      → removes both the leading and trailing '#'
 *  - CompareType_StartsWith  → removes the leading '#'
 *  - CompareType_EndsWith    → removes the trailing '#'
 *  - CompareType_Contains    → no change
 *
 * @param userAgentStr  The user-agent string to modify. Must be a writable,
 *                      null-terminated buffer.
 * @param compareType   The compare type previously determined by
 *                      detectCompareType().
 */
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
