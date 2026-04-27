#include "modApacheDeny_15Watt.h"
#include "shouldUserAgentBeBlocked.h"
#include "functionsString.h"

/**
 * Checks whether a given user-agent string matches any entry in the supplied
 * user-agent list.
 *
 * Iterates over all entries in @p moduleDataUserAgents and compares the
 * request's user-agent string against each stored pattern according to its
 * associated CompareType:
 *
 *  - CompareType_Contains   → true if the pattern appears anywhere in the
 *                             user-agent string (strstr).
 *  - CompareType_Equals     → true if the user-agent string is identical to
 *                             the pattern (strcmp).
 *  - CompareType_StartsWith → true if the user-agent string begins with the
 *                             pattern (stringStartsWith).
 *  - CompareType_EndsWith   → true if the user-agent string ends with the
 *                             pattern (stringEndsWith).
 *
 * This function is used for both the block list and the white-list. When used
 * with the white-list, a return value of true means the request is allowed
 * through; when used with the block list, it means the request should be
 * denied.
 *
 * @param serverRec             Apache request record (used for potential
 *                              future logging; currently not used directly).
 * @param userAgent             The user-agent string from the incoming request.
 *                              Must not be NULL.
 * @param moduleDataUserAgents  Pointer to the user-agent list to match against.
 *                              Must not be NULL.
 *
 * @return true   if the user-agent matches at least one entry in the list.
 * @return false  if no entry matches.
 */
bool shouldUserAgentBeBlocked(
    const request_rec *serverRec,
    const char *userAgent,
    const ModuleDataUserAgents  *moduleDataUserAgents
) {
    for (int i = 0; i < moduleDataUserAgents->cntUserAgents; i++) {

        switch (moduleDataUserAgents->userAgents[i].compareType) {
            case CompareType_Contains:
                if (NULL != strstr(userAgent, moduleDataUserAgents->userAgents[i].userAgent)) {
                    return true;
                }
                break;

            case CompareType_Equals:
                if (0 == strcmp(userAgent, moduleDataUserAgents->userAgents[i].userAgent)) {
                    return  true;
                }
                break;

            case CompareType_StartsWith:
                if (true == stringStartsWith(userAgent, moduleDataUserAgents->userAgents[i].userAgent)) {
                    return  true;
                }
                break;

            case CompareType_EndsWith:
                if (true == stringEndsWith(userAgent, moduleDataUserAgents->userAgents[i].userAgent)) {
                    return  true;
                }
                break;
        }
    }

    return false;
}
