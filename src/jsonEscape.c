#include "jsonEscape.h"
#include <string.h>

const char *jsonEscapeString(apr_pool_t *pool, const char *str)
{
    if (NULL == str) {
        return "";
    }

    const size_t len = strlen(str);
    char *result = apr_palloc(pool, 2 * len + 1);
    if (NULL == result) {
        return "";
    }

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        const unsigned char c = (unsigned char)str[i];

        if (c < 0x20) {                 // control chars incl. tab (0x09)
            result[j++] = ' ';
        } else {
            switch (c) {
                case '"':
                    result[j++] = '\\';
                    result[j++] = '"';
                    break;
                case '\\':
                    result[j++] = '\\';
                    result[j++] = '\\';
                    break;
                case '/':
                    result[j++] = '\\';
                    result[j++] = '/';
                    break;
                default:
                    result[j++] = (char)c;
                    break;
            }
        }
    }

    result[j] = '\0';
    return result;
}
