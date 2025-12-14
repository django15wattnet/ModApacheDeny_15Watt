// Programmiersprache: C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

/*
 * Prüft, ob eine IPv4-Adresse in einem IPv4-Netzwerkbereich (CIDR) liegt.
 * Beispiel: ip = "192.168.1.10", cidr = "192.168.1.0/24"
 */
int ipv4_in_cidr(const char *ip, const char *cidr) {
    char network[64];
    char *slash;
    int prefix_len;
    struct in_addr ip_addr, net_addr;
    uint32_t mask;

    // CIDR-String kopieren und in Netzadresse / Präfix splitten
    strncpy(network, cidr, sizeof(network) - 1);
    network[sizeof(network) - 1] = '\0';

    slash = strchr(network, '/');
    if (!slash) {
        fprintf(stderr, "Ungültiges IPv4-CIDR-Format: %s\n", cidr);
        return 0;
    }

    *slash = '\0';
    prefix_len = atoi(slash + 1);
    if (prefix_len < 0 || prefix_len > 32) {
        fprintf(stderr, "Ungültige Präfixlänge: %d\n", prefix_len);
        return 0;
    }

    // IPv4-Adressen parsen
    if (inet_pton(AF_INET, ip, &ip_addr) != 1) {
        fprintf(stderr, "Ungültige IPv4-Adresse: %s\n", ip);
        return 0;
    }
    if (inet_pton(AF_INET, network, &net_addr) != 1) {
        fprintf(stderr, "Ungültige IPv4-Netzadresse: %s\n", network);
        return 0;
    }

    // Maske erstellen
    if (prefix_len == 0) {
        mask = 0;
    } else {
        mask = htonl(0xFFFFFFFFu << (32 - prefix_len));
    }

    // Vergleichen, ob IP & Maske == Netz & Maske
    return (ip_addr.s_addr & mask) == (net_addr.s_addr & mask);
}

/*
 * Prüft, ob eine IPv6-Adresse in einem IPv6-Netzwerkbereich (CIDR) liegt.
 * Beispiel: ip = "2001:db8::1", cidr = "2001:db8::/32"
 */
int ipv6_in_cidr(const char *ip, const char *cidr) {
    char network[128];
    char *slash;
    int prefix_len;
    struct in6_addr ip_addr6, net_addr6;
    int i;
    int full_bytes;
    int remaining_bits;

    // CIDR-String kopieren und in Netzadresse / Präfix splitten
    strncpy(network, cidr, sizeof(network) - 1);
    network[sizeof(network) - 1] = '\0';

    slash = strchr(network, '/');
    if (!slash) {
        fprintf(stderr, "Ungültiges IPv6-CIDR-Format: %s\n", cidr);
        return 0;
    }

    *slash = '\0';
    prefix_len = atoi(slash + 1);
    if (prefix_len < 0 || prefix_len > 128) {
        fprintf(stderr, "Ungültige Präfixlänge: %d\n", prefix_len);
        return 0;
    }

    // IPv6-Adressen parsen
    if (inet_pton(AF_INET6, ip, &ip_addr6) != 1) {
        fprintf(stderr, "Ungültige IPv6-Adresse: %s\n", ip);
        return 0;
    }
    if (inet_pton(AF_INET6, network, &net_addr6) != 1) {
        fprintf(stderr, "Ungültige IPv6-Netzadresse: %s\n", network);
        return 0;
    }

    // Anzahl voller Bytes und verbleibender Bits im Präfix
    full_bytes = prefix_len / 8;
    remaining_bits = prefix_len % 8;

    // Volle Bytes vergleichen
    for (i = 0; i < full_bytes; i++) {
        if (ip_addr6.s6_addr[i] != net_addr6.s6_addr[i]) {
            return 0;
        }
    }

    // Restbits vergleichen
    if (remaining_bits > 0) {
        unsigned char mask = (unsigned char)(0xFF << (8 - remaining_bits));
        if ((ip_addr6.s6_addr[full_bytes] & mask) !=
            (net_addr6.s6_addr[full_bytes] & mask)) {
            return 0;
        }
    }

    return 1;
}

/*
 * Hilfsfunktion: Ermittelt, ob ein String eine gültige IPv4 oder IPv6 Adresse ist.
 * Gibt AF_INET, AF_INET6 oder -1 zurück.
 */
int detect_ip_family(const char *ip) {
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

int main(void) {
    char ip[128];
    char cidr[128];

    printf("IP-Adresse eingeben (IPv4 oder IPv6): ");
    if (!fgets(ip, sizeof(ip), stdin)) {
        fprintf(stderr, "Fehler beim Einlesen der IP-Adresse.\n");
        return 1;
    }

    printf("Netzwerkbereich im CIDR-Format eingeben (z.B. 192.168.1.0/24 oder 2001:db8::/32): ");
    if (!fgets(cidr, sizeof(cidr), stdin)) {
        fprintf(stderr, "Fehler beim Einlesen des Netzwerkbereichs.\n");
        return 1;
    }

    // Newlines entfernen
    ip[strcspn(ip, "\n")] = '\0';
    cidr[strcspn(cidr, "\n")] = '\0';

    int family = detect_ip_family(ip);
    if (family == -1) {
        printf("Die eingegebene IP-Adresse ist weder eine gültige IPv4- noch eine gültige IPv6-Adresse.\n");
        return 1;
    }

    if (family == AF_INET) {
        // IPv4
        if (ipv4_in_cidr(ip, cidr)) {
            printf("Die IPv4-Adresse %s liegt im Netzwerkbereich %s.\n", ip, cidr);
        } else {
            printf("Die IPv4-Adresse %s liegt NICHT im Netzwerkbereich %s.\n", ip, cidr);
        }
    } else if (family == AF_INET6) {
        // IPv6
        if (ipv6_in_cidr(ip, cidr)) {
            printf("Die IPv6-Adresse %s liegt im Netzwerkbereich %s.\n", ip, cidr);
        } else {
            printf("Die IPv6-Adresse %s liegt NICHT im Netzwerkbereich %s.\n", ip, cidr);
        }
    }

    return 0;
}
