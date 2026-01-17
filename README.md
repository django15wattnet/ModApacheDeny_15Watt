# ModApacheDeny_15Watt
An Apache 2.4.x module to deny access by ip address, ip range or user agent.

## Description
This Apache module connects to a MySQL database to check if the incoming request's 
IP address or user agent is listed in the database as blocked. If a match is found, 
the module denies access to the requested resource, by sending a 403 response. \
The information about blocked IP addresses, IP ranges, and user agents is loaded
to memory from the database at apache server startup for efficient access during
request processing. \
Changes to the database (additions or removals of blocked entries) are
reflected in the module's in-memory data structures by a apache server
graceful restart. 

**This modules functionality is addes to all virtual hosts and directories of your
Apache webserver!**

This module is designed to help web administrators to protect their servers from 
unwanted traffic, such as bots, scrapers, or malicious users.

## Compilation instructions
At this Moment the module is only tested on Ubuntu 24.04 and Apache 2.4.x,
no binaries are provided.

### To compile the module on Ubuntu 24.04, use the following commands:
- apt install apache2-dev
- apt install libmysqlclient-dev
- git clone git@github.com:django15wattnet/ModApacheDeny_15Watt.git
- cd ModApacheDeny_15Watt/src
- `sudo apxs -I/usr/include/mysql -L/usr/lib/aarch64-linux-gnu -lmysqlclient -lz  -lzstd -lssl -lcrypto -lresolv -lm -n modApacheDeny_15Watt -I /lib/aarch64-linux-gnu -Wl,-rpath,/usr/lib/x86_64-linux-gnu -i -a -c modApacheDeny_15Watt.c checkIpAddr.c loadUserAgents.c loadIpNetworks.c functionsString.c shouldUserAgentBeBlocked.c`

This also installs the module to Apache.

### Starting the server in debug mode
```apachectl -e debug -X```

## Configuration instructions
You find a sample configuration file in the `config` folder named 
`modApacheDeny_15Watt.conf`. \
Copy this file to your Apache configuration folder, e.g. `/etc/apache2/conf-available/`, 
edit the file to fit your database settings and enable it with:
- `sudo a2enconf modApacheDeny_15Watt`

## Database structure
The module expects a MySQL/Maria database with two tables with the following 
structures:
- One table for blocked user agents, with a column names value (varchar 255 / text)
all other columns are optional.

- One table for blocked ip addresses, ip ranges and hostnames, 
with a column names value (varchar 255 / text) all other columns are optional.

## Data formats
The module supports the following data formats in the database:

| Data Type     | Format Example                          | Description                            |
|---------------|-----------------------------------------|----------------------------------------|
| IP V4 Address | 129.168.0.1                             | A single IPv4 address                  |
| IP V6 Address | 2001:0db8:85a3:0000:0000:8a2e:0370:7334 | A single IPv6 address                  |
| IP V4 Range   | 192.168.0.0/24                          | An IPv4 CIDR range                     |
| IP V6 Range   | 2001:0db8::/32                          | An IPv6 CIDR range                     |
| Hostname      | example.com                             | A hostname matched LIKE '%example.com' |
| User Agent    | uncoolBot                               | A user agent string matched LIKE '%uncoolBot%' |

## Version history
| Version | Date       | Description                        |
|---------|------------|------------------------------------|
| 0.0.1   | 2026-01-09 | Initial release with all basic features |

## Future plans
- restrict module to specific virtual hosts or directories
- create binary packages for easy installation
- add more logging options
- add more possible ways to load blocked entries (e.g. from flat files)
- add reload of blocked entries without apache restart
- add machine-readable log format
- add logging statistics
 
## License
Apache License Version 2.0, January 2004 \
http://www.apache.org/licenses/

## Author
django15Wattnet: https://github.com/django15wattnet/ \
Email: modapachedeny@15watt.net