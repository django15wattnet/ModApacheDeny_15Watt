//
// Created by Thomas Siemion on 16.01.26.
//

#ifndef MODAPACHEDENY_15WATT_CHECKUSERAGENT_H
#define MODAPACHEDENY_15WATT_CHECKUSERAGENT_H

#include <httpd.h>
#include "modApacheDeny_15Watt.h"

bool shouldUserAgentBeBlocked(
    const request_rec *r,
    const char *userAgent,
    const ModuleDataUserAgents  *moduleDataUserAgents
);

#endif // MODAPACHEDENY_15WATT_CHECKUSERAGENT_H