//
// Created by Thomas Siemion on 13.12.25.
//

#ifndef MODAPACHEDENY_15WATT_LOADUSERAGENTS_H
#define MODAPACHEDENY_15WATT_LOADUSERAGENTS_H

#include "modApacheDeny_15Watt.h"

int loadUserAgents(
    const ModuleConfig *moduleConfig,
    ModuleDataUserAgents **dataUserAgents,
    apr_pool_t *pool,
    const server_rec *serverRec
);
enum CompareType detectCompareType(const char *userAgentStr);
void cutCompareTypeMarkers(char *userAgentStr, const enum CompareType compareType);

#endif //MODAPACHEDENY_15WATT_LOADUSERAGENTS_H