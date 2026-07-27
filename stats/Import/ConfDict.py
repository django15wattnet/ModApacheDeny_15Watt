
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
