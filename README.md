# ModApacheDeny_15Watt
An Apache module to deny access by ip address, ip range and / or user agent.

## Compilation Instructions
To compile the module, use the following command:

`apxs -I/usr/include/mysql -L/usr/lib/aarch64-linux-gnu -lmysqlclient -lz  -lzstd -lssl -lcrypto -lresolv -lm -n modApacheDeny_15Watt -I /lib/aarch64-linux-gnu -Wl,-rpath,/usr/lib/x86_64-linux-gnu -i -a -c modApacheDeny_15Watt.c checkIpAddr.c loadUserAgents.c`
