/**
 * Various string functions.
 * Created by Thomas Siemion on 26.12.25.
 */
#include "functionsString.h"

/**
 * Checks whether a string ends with a given suffix.
 *
 * @param str    The string to examine. May not be NULL.
 * @param suffix The suffix to look for. May not be NULL.
 * @return true  if @p str ends with @p suffix.
 * @return false if either argument is NULL, or @p suffix is longer than @p str,
 *               or the suffix is not found at the end of @p str.
 */
bool stringEndsWith(const char *str, const char *suffix)
{
    if (NULL == str || NULL == suffix) {
        return false;
    }

    const size_t strLen = strlen(str);
    const size_t suffixLen = strlen(suffix);

    if (suffixLen > strLen) {
        return false;
    }

    return strncmp(str + strLen - suffixLen, suffix, suffixLen) == 0;
}


/**
 * Checks whether a string starts with a given prefix.
 *
 * @param str    The string to examine. May not be NULL.
 * @param prefix The prefix to look for. May not be NULL.
 * @return true  if @p str starts with @p prefix.
 * @return false if either argument is NULL, or @p prefix is longer than @p str,
 *               or the prefix is not found at the beginning of @p str.
 */
bool stringStartsWith(const char *str, const char *prefix)
{
    if (NULL == str || NULL == prefix) {
        return false;
    }

    const size_t strLen = strlen(str);
    const size_t prefixLen = strlen(prefix);

    if (prefixLen > strLen) {
        return false;
    }

    return strncmp(str, prefix, prefixLen) == 0;
}


/**
 * Removes the last character of a string in-place.
 *
 * The last character is replaced with a null terminator ('\\0').
 * If the string is empty, the function does nothing.
 *
 * @param str  The string to modify. Must be a writable, null-terminated buffer.
 */
void stringDeleteCharRight(char *str)
{
    const size_t len = strlen(str);
    if (len > 0) {
        str[len - 1] = '\0';
    }
}


/**
 * Removes the first character of a string in-place.
 *
 * All remaining characters (including the null terminator) are shifted one
 * position to the left using memmove. If the string is empty, the function
 * does nothing.
 *
 * @param str  The string to modify. Must be a writable, null-terminated buffer.
 */
void stringDeleteCharLeft(char *str)
{
    const size_t len = strlen(str);
    if (len > 0) {
        memmove(str, str + 1, len); // including null terminator
    }
}
