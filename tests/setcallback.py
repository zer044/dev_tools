import requests
import json
from enum import Enum


class CallbackType(Enum):
    OBJECTS = "objects"
    IMAGE = "image"
    HEARTBEAT = "heartbeat"


class RobotCallback:
    def __init__(self, ip, port, url, callback_type):
        self.ip = ip
        self.port = port
        self.url = url
        self.callback_type = callback_type

    def send_callback(self, gmu_ip, gmu_port):
        # Construct the callback data
        callback_data = {
            'IP': self.ip,
            'port': self.port,
            'url': self.url,
            'type': self.callback_type
        }

        # Convert the callback data to JSON string
        callback_data_json = json.dumps(callback_data)

        # Send the callback data to the GMU server
        callback_url = f'http://{gmu_ip}:{gmu_port}/robot/set-callback'
        response = requests.post(callback_url, data=callback_data_json, headers={'Content-Type': 'application/json'})
        response.raise_for_status()
        response_body = response.content.decode('utf-8')
        return response_body


if __name__ == '__main__':
    callback_server_data = {
        'IP': '10.9.0.56',
        'port': 3333,
        'url': '/robot/set-image',
        'type': CallbackType.IMAGE.value
    }

    gmu_ip = '10.9.0.17'
    gmu_port = '8080'

    callback = RobotCallback(callback_server_data['IP'], callback_server_data['port'], callback_server_data['url'], callback_server_data['type'])
    response_body = callback.send_callback(gmu_ip, gmu_port)
    print(response_body)
