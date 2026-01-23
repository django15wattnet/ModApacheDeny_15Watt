//
// Created by Thomas Siemion on 21.01.26.
//

#ifndef MODAPACHEDENY_15WATT_LOADUSERAGENTSWL_H
#define MODAPACHEDENY_15WATT_LOADUSERAGENTSWL_H

#include "modApacheDeny_15Watt.h"

int loadUserAgentsWl(
    const ModuleConfig *moduleConfig,
    ModuleDataUserAgents **dataUserAgentsWl,
    apr_pool_t *pool,
    const server_rec *serverRec
);

#endif //MODAPACHEDENY_15WATT_LOADUSERAGENTSWL_H