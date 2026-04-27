#ifndef MODAPACHEDENY_15WATT_CHECKUSERAGENT_H
#define MODAPACHEDENY_15WATT_CHECKUSERAGENT_H

#include <httpd.h>
#include "modApacheDeny_15Watt.h"

bool shouldUserAgentBeBlocked(
    const request_rec *serverRec,
    const char *userAgent,
    const ModuleDataUserAgents  *moduleDataUserAgents
);

#endif // MODAPACHEDENY_15WATT_CHECKUSERAGENT_H