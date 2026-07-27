import json
import os

from Import.ConfDict import ConfDict


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
