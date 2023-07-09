import requests
import json

# A class that wraps the GMU API calls
class GPAPI:
    def __init__(self, gmu_ip, gmu_port):
        self.gmu_ip = gmu_ip
        self.gmu_port = gmu_port

    def setspeed(self, speed):
        url = f'http://{self.gmu_ip}:{self.gmu_port}/robot/set-speed'
        # Construct the data
        data = {
            'v': speed
        }

        # Convert the data to JSON string
        data_json = json.dumps(data)

        # Send the data to the GMU server
        response = requests.post(url, data=data_json, headers={'Content-Type': 'application/json'})
        response.raise_for_status()
        response_body = response.content.decode('utf-8')
        return response_body