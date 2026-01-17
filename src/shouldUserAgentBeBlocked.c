//
// Created by Thomas Siemion on 16.01.26.
//
#include "modApacheDeny_15Watt.h"
#include "shouldUserAgentBeBlocked.h"
#include "functionsString.h"

/**
 *
 * @return int
 */
bool shouldUserAgentBeBlocked(
    const request_rec *r,
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
