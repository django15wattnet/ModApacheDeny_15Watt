# ModApacheDeny_15Watt
An Apache 2.4.x module to deny access by ip address, ip range or user agent.

## Description
This Apache module reads data from a MySQL database to check if the incoming request's 
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
- `sudo apxs -I/usr/include/mysql -L/usr/lib/aarch64-linux-gnu -lmysqlclient -lz  -lzstd -lssl -lcrypto -lresolv -lm -n modApacheDeny_15Watt -I /lib/aarch64-linux-gnu -Wl,-rpath,/usr/lib/x86_64-linux-gnu -i -a -c modApacheDeny_15Watt.c checkIpAddr.c loadUserAgents.c loadUserAgentsWhiteList.c loadIpNetworks.c functionsString.c shouldUserAgentBeBlocked.c blockHash.c status.c`

This also installs the module to Apache.

### Starting the server in debug mode
```apachectl -e debug -X```

## Configuration instructions
You find a sample configuration file in the `config` folder named 
`modApacheDeny_15Watt.conf`. \
Copy this file to your Apache configuration folder, e.g. `/etc/apache2/conf-available/`, 
edit the file to fit your database settings and create in `/etc/apache2/conf-enabled/`
a symbolic link to the configuration file. \
Enable the module with:
- `sudo a2enconf modApacheDeny_15Watt`

and restart the Apache server.

### Configuration directives
The module provides the following configuration directives:

| Directive Name          | Description                                                                   | default value                     |
|-------------------------|-------------------------------------------------------------------------------|-----------------------------------|
| modApacheDeny_15Watt_dbHost | Database host name or IP address                                              | localhost                         |
| modApacheDeny_15Watt_dbUser | Database user                                                                 | root                              |
| modApacheDeny_15Watt_dbPwd | Database password                                                             | (empty)                           |
| modApacheDeny_15Watt_dbPort | Database port                                                                 | 3306                              |
| modApacheDeny_15Watt_database| Database name                                                                 | test                              |
| modApacheDeny_15Watt_tableAddresses | Database table name ip-addresses and host to block                            | block_ip_address |
| modApacheDeny_15Watt_tableUserAgents | Database table name user agents to block                                      | block_user_agent |
| modApacheDeny_15Watt_tableUserAgentsWl | Database table name user agents to white list                                 | block_user_agent_white_list |
| modApacheDeny_15Watt_allowEmptyUserAgent | Are empty user agent strings are allowed                                      |  false |
| modApacheDeny_15Watt_useAllowedHash | Use a hash to store allowed user agents and ip combinations for faster access | true |
| modApacheDeny_15Watt_allowedHashEntryCount | Number of entries in the allowed hash (if enabled)                                | 1000 |

## Database structure
The module expects a MySQL/Maria database with two tables with the following 
structures:
- One table for blocked user agents, with a column names value (varchar 255 / text)
all other columns are optional.

- One table for blocked ip addresses, ip ranges and hostnames, 
with a column names value (varchar 255 / text) all other columns are optional.

- One table for whitelisted user agents, with a column names value (varchar 255 / text)
all other columns are optional.

## Data formats
The module supports the following data formats in the database:

| Data Type                            | Format Example                          | Description                                |
|--------------------------------------|-----------------------------------------|--------------------------------------------|
| IP V4 Address                        | 129.168.0.1                             | A single IPv4 address                      |
| IP V6 Address                        | 2001:0db8:85a3:0000:0000:8a2e:0370:7334 | A single IPv6 address                      |
| IP V4 Range                          | 192.168.0.0/24                          | An IPv4 CIDR range                         |
| IP V6 Range                          | 2001:0db8::/32                          | An IPv6 CIDR range                         |
| Hostname                             | example.com                             | A hostname matched<br>LIKE '%example.com'  |
| User Agent <br/>User Agent whitelist |                                         |                                            |
|                                      | Abc                                     | Matches any user agent containing 'Abc'    |
|                                      | #Abc                                    | Matches any user agent starting with 'Abc' |
|                                      | Abc#                                    | Matches any user agent ending with 'Abc'   |
|                                      | #Abc#                                   | Matches any user agent exactly 'Abc'       

## Endpoint for status information
The module provides an endpoint to get the number of entries, the 10 newest and oldest in the blockHash as a JSON-structure. \
To enable this endpoint, add the following configuration to your Apache configuration file:
```
<Location /yourStatusEndpoint>
  SetHandler mod_apache_deny_15watt_status
  . . .
</Location>
```
## Version history
| Version | Date       | Description                                                                                                        |
|---------|------------|--------------------------------------------------------------------------------------------------------------------|
| 0.0.1   | 2026-01-09 | Initial release with all basic features                                                                            |
| 0.1.0   | 2026-01-17 | Added type of string compare to check if user agent should be blocked                                              |
| 0.1.1   | 2026-01-18 | Completion of the documentation                                                                                    |
| 0.2.0   | 2026-01-23 | Added support for user agent whitelisted                                                                           |
| 0.2.1   | 2026-01-28 | Completion of the documentation                                                                                    |
| 0.3.0   | 2026-04-27 | Added support for allowed combinations of ip-address and user agent by a hash to speed up access checks            |
| 0.4.0   | 2026-05-01 | Added a status endpoint to get the number of entries, the 10 newest and oldest in the blockHash asa JSON-structure |


## Future plans
- ~~add whitelist user agents (for example for Let's encrypt bot)~~
- add support for PostgreSQL database
- add support for SQLite database
- add more possible ways to load blocked entries (e.g. from flat/csv files)
- restrict module to specific virtual hosts or directories
- create binary packages for easy installation
- add more logging options
- add reload of blocked entries without apache restart
- add machine-readable log format
- add logging statistics
- ~~add a temporary caching of blocked entries to reduce search load (apr_hash_*)~~
    - ~~the keys can be ip addresses or user agents~~
 
## License
Apache License Version 2.0, January 2004 \
http://www.apache.org/licenses/

## Author
[Thomas Siemion](https://thomas.siemion.photography/) \
django15Wattnet: https://github.com/django15wattnet/ \
Email: [modapachedeny@15watt.net](modapachedeny@15watt.net)