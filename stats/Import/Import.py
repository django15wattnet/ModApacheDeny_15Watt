import fnmatch
import os
import re
import sys
import traceback

from Import.ConfDict import ConfDict
from Import.LogFile import LogFile


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
                try:
                    logLine.writeToJson()
                except Exception as e:
                    print(f"Error: {e} in {(tb := traceback.extract_tb(sys.exc_info()[2])[-1]).filename} at line {tb.lineno}")


    def __get(self, timeType: str, timeStr: str) -> str:
        # Don't know what my idea was here
        return ''
    

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
