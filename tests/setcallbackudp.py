import requests
import json


class RobotCallback:
    def __init__(self, ip, port):
        self.ip = ip
        self.port = port

    def send_callback(self, gmu_ip, gmu_port):
        # Construct the callback data
        callback_data = {
            'IP': self.ip,
            'port': self.port
        }

        # Convert the callback data to JSON string
        callback_data_json = json.dumps(callback_data)

        # Send the callback data to the GMU server
        callback_url = f'http://{gmu_ip}:{gmu_port}/robot/set-udp-callback'
        response = requests.post(callback_url, data=callback_data_json,
                                 headers={'Content-Type': 'application/json'})
        response.raise_for_status()
        response_body = response.content.decode('utf-8')
        return response_body


if __name__ == '__main__':
    callback_server_data = {
        'IP': '192.168.1.42',
        'port': 1234
    }

    gmu_ip = '192.168.1.205'
    gmu_port = '8080'

    callback = RobotCallback(callback_server_data['IP'],
                             callback_server_data['port'])
    response_body = callback.send_callback(gmu_ip, gmu_port)
    print(response_body)
