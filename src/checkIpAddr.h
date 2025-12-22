//
// Created by Thomas Siemion on 02.12.25.
//

#ifndef MODAPACHEDENY_15WATT_CHECKIPADDR_H
#define MODAPACHEDENY_15WATT_CHECKIPADDR_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <arpa/inet.h>

#define addressType_Unknown (-1)
#define addressType_IPv4      1
#define addressType_IPv6      2
#define addressType_CidrIPv4  3
#define addressType_CidrIPv6  4
#define addressType_Hostname  5

#define addressType_RegExHostname "^[A-Za-z0-9]([A-Za-z0-9-]{0,61}[A-Za-z0-9])?(\\.[A-Za-z0-9]([A-Za-z0-9-]{0,61}[A-Za-z0-9])?)*$"

/* Structure to hold IPv4 network information */
typedef struct {
    uint32_t network;
    uint32_t mask;
} NetInfoIpV4;

/*
 * @todo Strukturen für IPv6 Netzwerke erstellen
 */


int detectIpFamily(const char *ip);
int detectAddressType(const char *addr);
int compileIpV4Cidr(const char *cidr, NetInfoIpV4 *netInfoIpV4);

#endif //MODAPACHEDENY_15WATT_CHECKIPADDR_H