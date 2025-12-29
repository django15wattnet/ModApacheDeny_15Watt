# ModApacheDeny_15Watt
An Apache module to deny access by ip address, ip range and / or user agent.

## Compilation Instructions
To compile the module on Ubuntu 24.04, use the following commands:

- apt install apache2-dev
- apt install libmysqlclient-dev
- git clone git@github.com:django15wattnet/ModApacheDeny_15Watt.git
- cd ModApacheDeny_15Watt/src
- `apxs -I/usr/include/mysql -L/usr/lib/aarch64-linux-gnu -lmysqlclient -lz  -lzstd -lssl -lcrypto -lresolv -lm -n modApacheDeny_15Watt -I /lib/aarch64-linux-gnu -Wl,-rpath,/usr/lib/x86_64-linux-gnu -i -a -c modApacheDeny_15Watt.c checkIpAddr.c loadUserAgents.c loadIpNetworks.c functionsString.c`



## ToDos
- maschinell auswertbares Logformat erstellen
  - Idee: `ap_log_rerror(
                APLOG_MARK,
                APLOG_INFO,
                0,
                r,
                "modApacheDeny_15Watt host = %s %s %s",
                r->useragent_host, r->useragent_ip, r->useragent_addr->hostname
                );`