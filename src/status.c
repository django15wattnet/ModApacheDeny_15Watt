#include "status.h"
#include "blockHash.h"
#include "httpd.h"
#include "http_core.h"
#include "http_protocol.h"
#include "apr_hash.h"
#include "apr_strings.h"

int statusHandler(request_rec *r)
{
    // Not our handler
    if (strcmp(r->handler, "mod_apache_deny_15watt_status")) {
        return DECLINED;
    }

    if (r->method_number != M_GET) {
        return HTTP_METHOD_NOT_ALLOWED;
    }

    ap_set_content_type(r, "application/json");

    ap_rputs("{\n", r);
    ap_rprintf(r, "  \"blockHashEntryCount\": %d,\n", blockHashGetEntryCount());

    BlockHashEntry* oldest_entries[10];
    BlockHashEntry* newest_entries[10];
    int num_oldest, num_newest;

    blockHashGetOldestAndNewestEntries(oldest_entries, &num_oldest, newest_entries, &num_newest);

    ap_rputs("  \"newest_entries\": [\n", r);
    for (int i = 0; i < num_newest; i++) {
        char time_str[64];
        time_t ts = newest_entries[i]->tsLastUse;
        struct tm *tminfo = gmtime(&ts);
        strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%SZ", tminfo);

        const char *key = newest_entries[i]->key;
        const char *separator = strchr(key, '|');
        char *userAgent = "";
        const char *ipAddress = "";

        if (separator) {
            userAgent = apr_pstrndup(r->pool, key, separator - key);
            ipAddress = separator + 1;
        } else {
            userAgent = (char*)key;
        }

        ap_rprintf(
            r, "    "
            "{\"userAgent\": \"%s\", \"ipAddress\": \"%s\", \"tsLastUse\": \"%s\", \"doBlock\": %s, \"blockType\": \"%s\"}%s\n",
           userAgent,
           ipAddress,
           time_str,
           newest_entries[i]->doBlock ? "true" : "false",
           BlockTypeStrings[newest_entries[i]->blockType],
           i < num_newest - 1 ? "," : ""
        );
    }
    ap_rputs("  ],\n", r);

    ap_rputs("  \"oldest_entries\": [\n", r);
    for (int i = 0; i < num_oldest; i++) {
        char time_str[64];
        time_t ts = oldest_entries[i]->tsLastUse;
        struct tm *tminfo = gmtime(&ts);
        strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%SZ", tminfo);

        const char *key = oldest_entries[i]->key;
        const char *separator = strchr(key, '|');
        char *userAgent = "";
        const char *ipAddress = "";

        if (separator) {
            userAgent = apr_pstrndup(r->pool, key, separator - key);
            ipAddress = separator + 1;
        } else {
            userAgent = (char*)key;
        }

        ap_rprintf(
            r,
            "    {\"userAgent\": \"%s\", \"ipAddress\": \"%s\", \"tsLastUse\": \"%s\", \"doBlock\": %s, \"blockType\": \"%s\"}%s\n",
           userAgent,
           ipAddress,
           time_str,
           oldest_entries[i]->doBlock ? "true" : "false",
           BlockTypeStrings[oldest_entries[i]->blockType],
           i < num_oldest - 1 ? "," : ""
        );
    }
    ap_rputs("  ]\n", r);

    ap_rputs("}\n", r);

    return OK;
}
