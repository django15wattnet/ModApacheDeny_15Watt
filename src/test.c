#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <regex.h>

#include "checkIpAddr.h"


int main(void)
{
    char *ip = "192.168.0.99";
    const char *cidr = "192.168.0.0/24";
    const char *ipV6 = "2a03:4000:3b:205::2";
    NetInfoIpV4 netInfoIpV4;

    const int ret = compileIpV4Cidr(cidr, &netInfoIpV4);

    printf("sizeof(netInfoIpV4) = %lu\n", sizeof(netInfoIpV4));
    printf("compileIpV4Cidr ret=%d network = %u mask = %u\n", ret, netInfoIpV4.network, netInfoIpV4.mask);

    printf("%s = %d -- %s = %d\n", ip, detectIpFamily(ip), cidr, detectIpFamily(cidr));

    printf("%s = %d\n", ipV6, detectIpFamily(ipV6));

    const char *addrs[6];
    addrs[0] = "192.168.0.99";
    addrs[1] = "2001:db8::1";
    addrs[2] = "192.168.0.0/24";
    addrs[3] = "2001:db8::/32";
    addrs[4] = "thomas.siemion.photography";
    addrs[5] = "invalid_address";

    for (int i = 0; i < 6; i++) {
        const char *addr = addrs[i];
        const int type = detectAddressType(addr);
        printf("Address: %s Type: %d\n", addr, type);
    }

    return 0;
}
