//
// Created by Thomas Siemion on 03.12.25.
//
#include "checkIpAddr.h"


/**
 * Detects the IP family of the given IP address string.
 * Returns AF_INET for IPv4, AF_INET6 for IPv6, or -1 if invalid.
 *
 * @param ip
 * @return int
 */
int detectIpFamily(const char *ip)
{
    struct in_addr addr4;
    struct in6_addr addr6;

    if (inet_pton(AF_INET, ip, &addr4) == 1) {
        return AF_INET;
    }

    if (inet_pton(AF_INET6, ip, &addr6) == 1) {
        return AF_INET6;
    }

    return -1;
}


/**
 * Detects the type of the given address string.
 * Returns one of the addressType_XXX constants.
 *
 * @param addr
 * @return int
 */
int detectAddressType(const char *addr)
{
    switch (detectIpFamily(addr)) {
        case AF_INET:
            return addressType_IPv4;
        case AF_INET6:
            return addressType_IPv6;
        default:
            break;
    }

    // Check for CIDR notation
    char network[64];

    strncpy(network, addr, sizeof(network) - 1);
    network[sizeof(network) - 1] = '\0';

    char *slash = strchr(network, '/');
    if (NULL != slash) {
        // V4 or V6 CIDR
        *slash = '\0';
        switch (detectIpFamily(network)) {
            case AF_INET:
                return addressType_CidrIPv4;
            case AF_INET6:
                return addressType_CidrIPv6;
            default:
                return addressType_Unknown;
        }
    }

    // Check for valid hostname
    regex_t regex;
    if (0 != regcomp(&regex, addressType_RegExHostname, REG_EXTENDED | REG_NOSUB)) {
        // Should not happen
        return 99; //addressType_Unknown;
    }

    if (0 == regexec(&regex, addr, 0, NULL, 0)) {
        return addressType_Hostname;
    }

    return addressType_Unknown;
}


/**
 * Compiles an IPv4 CIDR string into network and mask.
 *
 * @param cidr
 * @param netInfoIpV4
 * @return int 1 on success, 0 on failure
 */
int compileIpV4Cidr(const char *cidr, NetInfoIpV4 *netInfoIpV4)
{
    char network[64];
    struct in_addr addrNet;

    // Split CIDR string in net address and prefix
    strncpy(network, cidr, sizeof(network) - 1);
    network[sizeof(network) - 1] = '\0';

    char *slash = strchr(network, '/');
    if (!slash) {
        // Invalid IPv4-CIDR-Format
        return 0;
    }

    *slash = '\0';
    const int lenPrefix = atoi(slash + 1);
    if (lenPrefix < 0 || lenPrefix > 32) {
        // Invalid prefix length
        return 0;
    }

    if (inet_pton(AF_INET, network, &addrNet) != 1) {
        // Invalid IPv4 network address
        return 0;
    }

    // Create the netmask
    if (lenPrefix == 0) {
        netInfoIpV4->mask = 0;
    } else {
        netInfoIpV4->mask = htonl(0xFFFFFFFFu << (32 - lenPrefix));
    }

    netInfoIpV4->network = addrNet.s_addr & netInfoIpV4->mask;

    return 1;
}