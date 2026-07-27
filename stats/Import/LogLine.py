import json
import re
from datetime import datetime
from sys import prefix

from Import.ConfDict import ConfDict
from Import.LogLineType import LogLineType


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
        if self.__type == LogLineType.ClientInfo:
            # Client info lines are not processed
            return
        
        # Is the line blocked or not
        if 'action' in self.__components:
            if 'blocked' == self.__components['action']:
                blockPrefix = 'bl'
            else:
                blockPrefix = 'nb'
        else:
            print(self.__components)
            return
        
        # First write the log line to the day JSON file
        nameJsonFile = f"{self.__config['pathStorage']}/{blockPrefix}_{self.dayString}.json"
        self._writeToFile(nameJsonFile)

        nameJsonFile = f"{self.__config['pathStorage']}/{blockPrefix}_{self.monthString}.json"
        self._writeToFile(nameJsonFile)


    def _writeToFile(self, nameJsonFile: str) -> None:
        with open(nameJsonFile, 'a+', encoding='utf-8') as handleJsonFile:
            # Move to start so the file can be read immediately if needed
            handleJsonFile.seek(0)
            content = handleJsonFile.read()

            if '' == content:
                data = {}
            else:
                try:
                    data = json.loads(content)
                except Exception as e:
                    print(f"JSON load Error: {e}")
                    data = {}

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
            
            try:
                json.dump(data, handleJsonFile, indent=4)
            except Exception as e:
                print(f"JSON dump Error: {e}")


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
            elif 'by ipv4 cidr=' in self.__line:
                self.__components['block_reason'] = LogLineType.BlockCdirIpV4
                self.__type = LogLineType.BlockCdirIpV4
            elif 'by ipv4=' in self.__line:
                self.__components['block_reason'] = LogLineType.B
                self.__type = LogLineType.BlockCdirIpV4
            elif 'by ipv6=' in self.__line:
                self.__components['block_reason'] = LogLineType.BlockIpV6
                self.__type = LogLineType.BlockIpV6
            elif 'by ipv6 cidr=' in self.__line:
                self.__components['block_reason'] = LogLineType.BlockCdirIpV6
                self.__type = LogLineType.BlockCdirIpV6
            elif 'by empty user agent' in self.__line:
                self.__components['block_reason'] = LogLineType.BlockEmptyUserAgent
                self.__type = LogLineType.BlockEmptyUserAgent
            else:
                self.__components['block_reason'] = LogLineType.Unknown
                self.__type = LogLineType.Unknown

        elif 'modApacheDeny_15Watt blockHashAddEntry' in self.__line:
            self.__components['action'] = 'blockHashAddEntry'
            self.__type = LogLineType.BlockNone
        
        elif 'modApacheDeny_15Watt white listed client by user agent' in self.__line:
            self.__components['action'] = 'white listed client by user agent'
            self.__type = LogLineType.BlockNone
        
        elif 'modApacheDeny_15Watt allowed client by user agent' in self.__line:
            self.__components['action'] = 'allowed client by user agent'
            self.__type = LogLineType.BlockNone
        elif 'modApacheDeny_15Watt client info' in self.__line:
            self.__components['action'] = 'client_info'
            self.__type = LogLineType.ClientInfo
        else:
            self.__components['action'] = 'not detected'
            self.__type = LogLineType.BlockNone
            
        
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
        else:
            self.__components['ip'] = None

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
        else:
            self.__components['referer'] = None


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
