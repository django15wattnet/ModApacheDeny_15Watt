from Import.ConfDict import ConfDict
from Import.LogLine import LogLine
from Import.LogLineType import LogLineType


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
