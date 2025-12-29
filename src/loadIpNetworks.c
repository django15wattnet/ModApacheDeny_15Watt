//
// Created by Thomas Siemion on 19.12.25.
//

#include "loadIpNetworks.h"

int loadIpNetworks(
    const ModuleConfig *moduleConfig,
    ModuleDataHostnames **dataHostnames,
    ModuleDataIpV4 **dataIpV4,
    ModuleDataIpV6 **dataIpV6,
    ModuleDataNetInfoIpV4 **dataNetInfoIpV4,
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
    snprintf(query, sizeof(query), "SELECT value FROM %s", moduleConfig->tableAddresses);

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

    // Get the counts of IpV4-, IpV6-Adressen und Hostnamen
    // and allocate the data structures
    int cntNetInfoIpV4 = 0;
    int cntIpV4        = 0;
    int cntIpV6        = 0;
    int cntHostnames   = 0;

    while ((row = mysql_fetch_row(res))) {
        int addrType = detectAddressType(row[0]);
        switch (addrType) {
            case addressType_CidrIPv4:
                cntNetInfoIpV4++;
                break;
            case addressType_IPv4:
                cntIpV4++;
                break;
            case addressType_IPv6:
                cntIpV6++;
                break;
            case addressType_Hostname:
                cntHostnames++;
                break;
            default:
                // Unknown address type
                ap_log_error(
                    APLOG_MARK,
                    APLOG_WARNING,
                    0,
                    serverRec,
                    "Unknown address type for value '%s' in table '%s'",
                    row[0],
                    moduleConfig->tableAddresses
                );
                break;
        }
    }

    mysql_data_seek(res, 0);

    // Allocate the necessary data structures
    // size of                         ↓ Integer     ↓ + count strings * size of pointer
    *dataHostnames = apr_pcalloc(pool, sizeof(int) + cntHostnames * sizeof(const char *));
    (*dataHostnames)->cntHostnames = cntHostnames;

    *dataNetInfoIpV4 = apr_pcalloc(pool, sizeof(int) + cntNetInfoIpV4 * sizeof(NetInfoIpV4 *));
    (*dataNetInfoIpV4)->cntNetInfoIpV4 = cntNetInfoIpV4;

    *dataIpV4 = apr_pcalloc(pool, sizeof(int) + cntIpV4 * sizeof(const char *));
    (*dataIpV4)->cntIpV4 = cntIpV4;

    *dataIpV6 = apr_pcalloc(pool, sizeof(int) + cntIpV6 * sizeof(const char *));
    (*dataIpV6)->cntIpV6 = cntIpV6;

    *dataHostnames = apr_palloc(pool, sizeof(int) + cntHostnames * sizeof(const char *));
    (*dataHostnames)->cntHostnames = cntHostnames;

    int idxHostnames   = 0;
    int idxIpV4        = 0;
    int idxIpV6        = 0;
    int idxNetInfoIpV4 = 0;

    NetInfoIpV4 *ptrNetInfoIpV4;

    while ((row = mysql_fetch_row(res))) {

        int addrType = detectAddressType(row[0]);
        switch (addrType) {
            case addressType_CidrIPv4:
                ptrNetInfoIpV4 = apr_pcalloc(pool, sizeof(NetInfoIpV4));
                compileIpV4Cidr(row[0], ptrNetInfoIpV4);
                ptrNetInfoIpV4->cidr = apr_pstrdup(pool, row[0]);
                (*dataNetInfoIpV4)->netInfoIpV4[idxNetInfoIpV4] = ptrNetInfoIpV4;
                idxNetInfoIpV4++;
                break;

            case addressType_IPv4:
                (*dataIpV4)->ipV4[idxIpV4] = apr_pstrdup(pool, row[0]);
                idxIpV4++;
                break;

            case addressType_IPv6:
                (*dataIpV6)->ipV6[idxIpV6] = apr_pstrdup(pool, row[0]);
                idxIpV6++;
                break;

            case addressType_Hostname:
                (*dataHostnames)->hostnames[idxHostnames] = apr_pstrdup(pool, row[0]);
                idxHostnames++;
                break;

            default:
                // Unknown address type
                break;
        }
    }

    mysql_free_result(res);
    mysql_close(conn);

    return 0;
}
