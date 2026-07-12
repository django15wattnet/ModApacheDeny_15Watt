#!/usr/bin/env python3

import argparse
import fnmatch
import json
import os
import re
from datetime import datetime
from enum import Enum


class ConfDict(dict):
    """
        Reads the .env file and behaves as a read-only dict,
        with correct types of the values.
    """
    
    def __init__(self, pathFileConfig: str):
        super().__init__()
        self.__pathFileConfig = pathFileConfig
        self.__parseConfig()
    
    @property
    def pathDotEnv(self) -> str:
        return self.__pathDotEnv1
    
    def __parseConfig(self):
        """
            Reads the dotenv self.__pathDotEnv and turns it into a dict
        """
        try:
            handleConfig = open(self.__pathFileConfig, 'r', encoding='utf-8')
        except FileNotFoundError:
            raise FileNotFoundError(f"DotEnv file not found: {self.__pathFileConfig}")
        except OSError as e:
            raise OSError(f"DotEnv file could not be opened: {self.__pathFileConfig} – {e}")
        
        with handleConfig as f:
            for line in f:
                line = line.strip()
                
                # Skip empty lines and comments
                if not line or line.startswith('#'):
                    continue
                
                # Only process lines containing '='
                if '=' not in line:
                    continue
                
                key, _, value = line.partition('=')
                key = key.strip()
                value = value.strip()
                
                # Strip surrounding quotes (single and double)
                if len(value) >= 2 and value[0] in ('"', "'") and value[-1] == value[0]:
                    value = value[1:-1]
                
                # Use dict.__setitem__ directly during initialization
                dict.__setitem__(self, key, self.__convertValue(value))
    
    
    def __convertValue(self, value: str):
        """
            Converts a string value from the dotenv file to the appropriate Python type.
            Order: bool -> int -> float -> str
        """
        if value.lower() == 'true':
            return True
        if value.lower() == 'false':
            return False
        try:
            return int(value)
        except ValueError:
            pass
        try:
            return float(value)
        except ValueError:
            pass
        return value
    
    
    # Make the dict read-only by blocking all mutating operations
    def __setitem__(self, key, value):
        raise TypeError("DotEnvToDict is read-only")
    
    
    def __delitem__(self, key):
        raise TypeError("DotEnvToDict is read-only")
    
    def pop(self, *args, **kwargs):
        raise TypeError("DotEnvToDict is read-only")
    
    
    def popitem(self):
        raise TypeError("DotEnvToDict is read-only")
    
    
    def clear(self):
        raise TypeError("DotEnvToDict is read-only")
    
    
    def update(self, *args, **kwargs):
        raise TypeError("DotEnvToDict is read-only")
    
    
    def setdefault(self, key, default=None):
        raise TypeError("DotEnvToDict is read-only")



class LogLineType(Enum):
    ClientInfo     = 1
    BlockNone      = 2
    BlockUserAgent = 3
    BlockCdirIpV4  = 4
    BlockCdirIpV6  = 5
    BlockHostName  = 6
    
    def __str__(self):
        """Returns a human-readable representation of the enum value"""
        mapping = {
            LogLineType.ClientInfo: 'Client-Information',
            LogLineType.BlockNone: 'Kein Block',
            LogLineType.BlockUserAgent: 'User Agent',
            LogLineType.BlockCdirIpV4: 'IPv4 CIDR',
            LogLineType.BlockCdirIpV6: 'IPv6 CIDR',
            LogLineType.BlockHostName: 'Hostname'
        }
        return mapping.get(self, self.name)



class LogFile(object):
    def __init__(self, pathLogFile: str, config: ConfDict):
        super().__init__()
        
        self.__config      = config
        self.__pathLogFile = pathLogFile
        self.__handle      = open(self.__pathLogFile, 'r', encoding='utf-8')
        self.__lines       = []
        
        # Iterate over lines
        for strLogLine in self.__handle.read().splitlines():
            # Process each line
            try:
                self.__lines.append(LogLine(line=strLogLine, config=self.__config))
                
            except ValueError as e:
                # Skip lines that do not match the ModApacheDeny format
                continue
        
        self.__handle.close()
    
    
    @property
    def lines(self):
        return self.__lines


    @property
    def namesDataFiles(self) -> list:
        """
            Returns a list of names for the data files that should be created based on the log lines.
            The names are based on the day and month of the log lines.
        """
        names = set()
        for logLine in self.__lines:
            if logLine.type == LogLineType.BlockNone:
                continue
            names.add(logLine.dayString + '.json')
            names.add(logLine.monthString + '.json')
        return sorted(names)



class LogLine(object):
    
    __type = LogLineType.BlockNone
    
    def __init__(self, line: str, config: ConfDict):
        super().__init__()
        
        if '] modApacheDeny_15Watt ' not in line:
            raise ValueError('No ModApacheDeny log line')
        
        self.__config = config
        self.__line = line
        self.__timestamp = None
        self.__components = {}
        
        self.__parseTimestamp()
        self.__parseLine()


    def writeToJson(self) -> None:
        # First write the log line to the day JSON file
        nameJsonFile = f"{self.__config['pathStorage']}/{self.dayString}.json"
        self._writeToFile(nameJsonFile)

        nameJsonFile = f"{self.__config['pathStorage']}/{self.monthString}.json"
        self._writeToFile(nameJsonFile)


    def _writeToFile(self, nameJsonFile: str) -> None:
        with open(nameJsonFile, 'a+', encoding='utf-8') as handleJsonFile:
            # Move to start so the file can be read immediately if needed
            handleJsonFile.seek(0)
            content = handleJsonFile.read()

            if '' == content:
                data = {}
            else:
                data = json.loads(content)

            if self.userAgent in data:
                data[self.userAgent]['count'] += 1
            else:
                data[self.userAgent] = {
                    'count': 1,
                    'ips':   {}
                }

            if self.clientIp and self.clientIp not in data[self.userAgent]['ips']:
                data[self.userAgent]['ips'][self.clientIp] = 1
            elif self.clientIp:
                data[self.userAgent]['ips'][self.clientIp] += 1

            # Move back to start and truncate the file before writing
            handleJsonFile.seek(0)
            handleJsonFile.truncate()
            json.dump(data, handleJsonFile, indent=4)

        handleJsonFile.close()
    
    
    @property
    def dayString(self) -> str:
        return self.__timestamp.strftime('%Y-%m-%d')
    
    
    @property
    def monthString(self) -> str:
        return self.__timestamp.strftime('%Y-%m')
    
    
    def __parseTimestamp(self):
        """
            Extracts the timestamp from the log line and converts it to a datetime object.
            Format: [Thu Jun 04 02:15:11.172710 2026]
        """
        # Regex to extract the timestamp
        pattern = r'\[([A-Za-z]{3}\s+[A-Za-z]{3}\s+\d{2}\s+\d{2}:\d{2}:\d{2}\.\d{6}\s+\d{4})\]'
        match = re.search(pattern, self.__line)
        
        if not match:
            raise ValueError(f"Could not extract timestamp from log line: {self.__line}")
        
        timestamp_str = match.group(1)
        
        # Parse the timestamp into a datetime object
        # Format: Thu Jun 04 02:15:11.172710 2026
        try:
            self.__timestamp = datetime.strptime(timestamp_str, '%a %b %d %H:%M:%S.%f %Y')
        except ValueError as e:
            raise ValueError(f"Could not parse timestamp '{timestamp_str}': {e}")
    
    
    def __parseLine(self):
        """
            Parse the log line and extract all components into a dictionary.
            
            Example log line:
            [Thu Jun 04 13:44:05.963758 2026] [:info] [pid 66452:tid 138422998668992]
            [client 35.254.244.123:28502] modApacheDeny_15Watt client info,
            user agent=Mozilla/5.0 (...) ip=35.254.244.123 host=(null), referer: https://aussichtslos.ms/
        """
        self.__components = {}

        # remove the optional refer part from the end of the log line
        self.__line = self.__line.split(', referer: ', 1)[0]
        
        # Extract client IP and port [client 35.254.244.123:28502]
        client_pattern = r'\[client\s+([\d:.a-f]+):(\d+)\]'
        client_match = re.search(client_pattern, self.__line, re.IGNORECASE)
        if client_match:
            self.__components['client_ip'] = client_match.group(1)
            self.__components['client_port'] = int(client_match.group(2))
        
        # Determine message type (blocked or client info)
        # todo: Determine allowed request too
        if 'modApacheDeny_15Watt blocked client' in self.__line:
            self.__components['action'] = 'blocked'
            
            # Extract block reason for blocked messages
            if 'by user agent=' in self.__line:
                self.__components['block_reason'] = LogLineType.BlockUserAgent
                self.__type = LogLineType.BlockUserAgent
            elif 'by hostname=' in self.__line:
                self.__components['block_reason'] = LogLineType.BlockHostName
                self.__type = LogLineType.BlockHostName
            elif 'by ipv4=' in self.__line:
                self.__components['block_reason'] = LogLineType.BlockCdirIpV4
                self.__type = LogLineType.BlockCdirIpV4
            elif 'by ipv6=' in self.__line:
                self.__components['block_reason'] = LogLineType.BlockCdirIpV6
                self.__type = LogLineType.BlockCdirIpV6
            
            self.__components['action'] = 'blocked'
        else:
            self.__components['action'] = 'client_info'
            self.__type = LogLineType.ClientInfo
        
        # Extract user agent
        user_agent_pattern = r'user agent=(.+?)(?:\s+(?:ip|host)=|\s*$)'
        user_agent_match = re.search(user_agent_pattern, self.__line)
        if user_agent_match:
            self.__components['user_agent'] = user_agent_match.group(1).strip()
        else:
            self.__components['user_agent'] = '-'
        
        # Extract IP from message body
        ip_pattern = r'(?:^|\s)ip=([\d:.a-f]+)'
        ip_match = re.search(ip_pattern, self.__line, re.IGNORECASE)
        if ip_match:
            self.__components['ip'] = ip_match.group(1)
        
        # Extract hostname from message body
        host_pattern = r'host=([^\s,]+)'
        host_match = re.search(host_pattern, self.__line)
        if host_match:
            hostname = host_match.group(1)
            self.__components['host'] = None if hostname == '(null)' else hostname
        
        # Extract referer (optional)
        referer_pattern = r'referer:\s+(.+?)(?:\s*$)'
        referer_match = re.search(referer_pattern, self.__line)
        if referer_match:
            self.__components['referer'] = referer_match.group(1).strip()
    
    
    @property
    def components(self) -> dict:
        """Returns the dictionary with all extracted components"""
        return self.__components
    
    
    @property
    def timestamp(self) -> datetime:
        """Returns the parsed timestamp as a datetime object"""
        return self.__timestamp
    
    
    @property
    def line(self) -> str:
        """Returns the original log line"""
        return self.__line
    
    
    @property
    def type(self) -> LogLineType:
        """Returns the LogLineType"""
        return self.__type


    @property
    def userAgent(self) -> str:
        """Returns the extracted user agent string, or an empty string if not present"""
        return self.__components.get('user_agent', '')


    @property
    def clientIp(self) -> str:
        """Returns the extracted client IP address, or None if not present"""
        return self.__components.get('client_ip')
    
    
    def __str__(self):
        """
            Creates a human-readable output from the extracted components
        """
        lines = []
        
        # Header with timestamp
        lines.append("=" * 80)
        lines.append(f"Log-Eintrag: {self.__timestamp.strftime('%d.%m.%Y %H:%M:%S.%f')[:-3]}")
        lines.append("=" * 80)
        
        # Action
        if 'action' in self.__components:
            action = self.__components['action']
            if action == 'blocked':
                lines.append(f"🚫 AKTION: Client wurde blockiert")
            elif action == 'client_info':
                lines.append(f"ℹ️  AKTION: Client-Information")
            else:
                lines.append(f"AKTION: {action}")
        
        # Block reason (if present)
        if 'block_reason' in self.__components:
            lines.append(f"   Block-Grund: {self.__components['block_reason']}")
        
        lines.append("")
        
        # Client information
        lines.append("Client-Informationen:")
        if 'client_ip' in self.__components:
            lines.append(f"  IP-Adresse: {self.__components['client_ip']}")
        if 'client_port' in self.__components:
            lines.append(f"  Port:       {self.__components['client_port']}")
        if 'ip' in self.__components and self.__components.get('ip') != self.__components.get('client_ip'):
            lines.append(f"  IP (Body):  {self.__components['ip']}")
        if 'host' in self.__components:
            host_val = self.__components['host'] if self.__components['host'] else '(nicht aufgelöst)'
            lines.append(f"  Hostname:   {host_val}")
        
        # User agent (if present)
        if 'user_agent' in self.__components:
            lines.append("")
            lines.append("User Agent:")
            ua = self.__components['user_agent']
            # Wrap long user-agent strings
            if len(ua) > 76:
                lines.append(f"  {ua[:76]}")
                lines.append(f"  {ua[76:]}")
            else:
                lines.append(f"  {ua}")
        
        # Referer (if present)
        if 'referer' in self.__components:
            lines.append("")
            lines.append(f"Referer: {self.__components['referer']}")
        
        lines.append("=" * 80)
        
        return "\n".join(lines)


class DataFile(object):
    """
        Represents a JSON data file with grouped data
    """
    def __init__(self, confDict: ConfDict, nameFile: str):
        self.__confDict = confDict
        self.__nameFile = nameFile


    def sort(self):
        """
            Sorts the data file by the count of blocked requests per user agent, in descending order.
            Also sorts the IP addresses for each user agent by their count in descending order.
        """
        pathFile = f"{self.__confDict['pathStorage']}/{self.__nameFile}"

        # Ensure file exists; create an empty JSON object if missing.
        if not os.path.exists(pathFile):
            with open(pathFile, 'w', encoding='utf-8') as handle:
                json.dump({}, handle, indent=4)
            return

        with open(pathFile, 'r+', encoding='utf-8') as handle:
            content = handle.read().strip()

            if not content:
                data = {}
            else:
                try:
                    data = json.loads(content)
                except json.JSONDecodeError as e:
                    raise ValueError(f"Invalid JSON in data file '{pathFile}': {e}")

            if not isinstance(data, dict):
                raise ValueError(f"Unexpected JSON structure in '{pathFile}': expected object at root")

            sorted_user_agents = sorted(
                data.items(),
                key=lambda item: item[1].get('count', 0) if isinstance(item[1], dict) else 0,
                reverse=True
            )

            sorted_data = {}
            for user_agent, user_data in sorted_user_agents:
                if not isinstance(user_data, dict):
                    sorted_data[user_agent] = user_data
                    continue

                ips = user_data.get('ips', {})
                if isinstance(ips, dict):
                    sorted_ips = dict(
                        sorted(
                            ips.items(),
                            key=lambda ip_item: ip_item[1] if isinstance(ip_item[1], int) else 0,
                            reverse=True
                        )
                    )
                else:
                    sorted_ips = {}

                sorted_data[user_agent] = {
                    **user_data,
                    'ips': sorted_ips
                }

            handle.seek(0)
            handle.truncate()
            json.dump(sorted_data, handle, indent=4)

class Import(object):
    __neededConfigKeys = ['pathLogFiles', 'filePattern', 'pathStorage']

    def __init__(self, pathFileConfig: str):
        super().__init__()

        self.pathFileConfig = pathFileConfig
        self.__config = ConfDict(pathFileConfig=pathFileConfig)
        self.__checkConfig()

        namesDataFiles = set()
        for pathLogFile in self.__getListLogFiles():
            logFile = LogFile(pathLogFile=pathLogFile, config=self.__config)

            namesDataFiles = namesDataFiles.union(set(logFile.namesDataFiles))

            for logLine in logFile.lines:
                if logLine.components['action'] != 'client_info':
                    print(logLine.components)
                    print('--------------------------')
                if logLine.components['action'] == 'blocked':
                    logLine.writeToJson()

        for n in namesDataFiles:
            dataFile = DataFile(confDict=self.__config, nameFile=n)
            dataFile.sort()

        for n in namesDataFiles:
            print(f"Data file created: {n}")

    def __get(self, timeType: str, timeStr: str) -> str:
        return

    def __checkConfig(self):
        """
            Checks if the config is valid.
            If not, raises an exception.
        """
        notPresent = []
        for key in self.__neededConfigKeys:
            if key not in self.__config:
                notPresent.append(key)

        if notPresent:
            raise KeyError(f"Missing config keys: {', '.join(notPresent)}")

        # Do the directories exist?
        pathLogFiles = self.__config['pathLogFiles']
        if not os.path.isdir(pathLogFiles):
            raise FileNotFoundError(
                f"Directory configured in 'pathLogFiles' does not exist: {pathLogFiles}"
            )

        pathStorage = self.__config['pathStorage']
        if not os.path.isdir(pathStorage):
            try:
                os.makedirs(pathStorage, exist_ok=True)
            except OSError as e:
                raise OSError(
                    f"Directory configured in 'pathStore' could not be created: {pathStorage} – {e}"
                ) from e

        if not os.path.isdir(pathStorage):
            raise NotADirectoryError(
                f"Path configured in 'pathStore' is not a directory: {pathStorage}"
            )

    def __getListLogFiles(self) -> list:
        """
            Returns a list of log file paths that match the configured file pattern.
            The file pattern can be a glob pattern (e.g., *.error.log.1) or a regex.

            Returns:
                list: List of absolute file paths matching the pattern
        """
        pathLogFiles = self.__config['pathLogFiles']
        filePattern = self.__config['filePattern']

        # Convert glob pattern to regex pattern
        # fnmatch.translate converts shell-style wildcards to regex
        try:
            regexPattern = fnmatch.translate(filePattern)
            pattern = re.compile(regexPattern)
        except re.error as e:
            raise ValueError(f"Invalid file pattern in 'filePattern': {filePattern} – {e}")

        matchingFiles = []

        # List all entries in the directory
        try:
            entries = os.listdir(pathLogFiles)
        except OSError as e:
            raise OSError(f"Could not read directory: {pathLogFiles} – {e}")

        # Filter files that match the pattern
        for entry in entries:
            fullPath = os.path.join(pathLogFiles, entry)

            # Only consider files (not directories)
            if os.path.isfile(fullPath):
                if pattern.match(entry):
                    matchingFiles.append(fullPath)

        return matchingFiles



if __name__ == "__main__":
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument(
        "-h", "--help",
        action="help",
        help="Shows this short help text and exits the program."
    )
    parser.add_argument(
        "--fileConfig", "-c",
        action="store",
        help="Full path to configuration file",
        required=True
    )
    args = parser.parse_args()
    fileConfig = args.fileConfig
    
    try:
        imp = Import(pathFileConfig=fileConfig)
    except Exception as e:
        print(e)
        exit(1)
    
    exit(0)
    
