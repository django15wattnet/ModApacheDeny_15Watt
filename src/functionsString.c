/**
 * Various string functions.
 * Created by Thomas Siemion on 26.12.25.
 */
#include "functionsString.h"

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
 * Deletes the last character of the string by replacing it with null terminator.
 */
void stringDeleteCharRight(char *str)
{
    const size_t len = strlen(str);
    if (len > 0) {
        str[len - 1] = '\0';
    }
}


/**
 * Deletes the first character of the string by shifting the rest to the left.
 */
void stringDeleteCharLeft(char *str)
{
    const size_t len = strlen(str);
    if (len > 0) {
        memmove(str, str + 1, len); // including null terminator
    }
}
