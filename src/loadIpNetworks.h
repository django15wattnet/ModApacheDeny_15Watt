//
// Created by Thomas Siemion on 19.12.25.
//

#ifndef MODAPACHEDENY_15WATT_LOADIPNETWORKS_H
#define MODAPACHEDENY_15WATT_LOADIPNETWORKS_H

#include "checkIpAddr.h"
#include "modApacheDeny_15Watt.h"

int loadIpNetworks(
    const ModuleConfig *moduleConfig,
    ModuleDataHostnames **dataHostnames,
    ModuleDataIpV4 **dataIpV4,
    ModuleDataIpV6 **dataIpV6,
    ModuleDataNetInfoIpV4 **dataNetInfoIpV4,
    ModuleDataNetInfoIpV6 **dataNetInfoIpV6,
    apr_pool_t *pool,
    const server_rec *serverRec
);

#endif //MODAPACHEDENY_15WATT_LOADIPNETWORKS_H