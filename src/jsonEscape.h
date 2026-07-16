#ifndef MODAPACHEDENY_15WATT_JSONESCAPE_H
#define MODAPACHEDENY_15WATT_JSONESCAPE_H

#include "apr_pools.h"

/**
 * Escapes a string so it can be safely used as a JSON string value.
 *
 * Control characters (U+0000..U+001F, including tab U+0009) are replaced
 * with a space, and all characters that may be quoted in JSON (", \\, /)
 * are quoted.
 *
 * @param pool The APR pool used to allocate the returned string.
 * @param str  The input string. May be NULL.
 * @return     A newly allocated, JSON-safe version of @p str.
 */
const char *jsonEscapeString(apr_pool_t *pool, const char *str);

#endif //MODAPACHEDENY_15WATT_JSONESCAPE_H
