
from enum import Enum


class LogLineType(Enum):
    Unknown             = 0
    ClientInfo          = 1
    BlockNone           = 2
    BlockUserAgent      = 3
    BlockIpV4           = 4
    BlockCdirIpV4       = 5
    BlockIpV6           = 6
    BlockCdirIpV6       = 7
    BlockHostName       = 8
    BlockEmptyUserAgent = 9

    def __str__(self):
        """Returns a human-readable representation of the enum value"""
        mapping = {
            LogLineType.ClientInfo:          'Client-Information',
            LogLineType.BlockNone:           'Kein Block',
            LogLineType.BlockUserAgent:      'User Agent',
            LogLineType.BlockCdirIpV4:       'IPv4 CIDR',
            LogLineType.BlockCdirIpV6:       'IPv6 CIDR',
            LogLineType.BlockHostName:       'Hostname',
            LogLineType.BlockEmptyUserAgent: 'Empty User Agent',
            LogLineType.Unknown:             'Unknown'
        }
        return mapping.get(self, self.name)
