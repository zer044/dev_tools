import requests
import json

# A class that wraps the GMU API calls
class GPAPI:
    """
    A class that wraps the Greyparrot robot API calls
    """
    def __init__(self, gmu_ip, gmu_port):
        self.gmu_ip = gmu_ip
        self.gmu_port = gmu_port

    def set_speed(self, speed):
        """
        Sets the speed of the conveyor belt, used for tracking
        """
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


    def get_image_raw(self):
        """
        Gets the raw image from the robot as a base 64 encoded string
        """
        url = f'http://{self.gmu_ip}:{self.gmu_port}/get_raw_image'
        response = requests.get(url, timeout=1)

        return response.content

    def set_robot_conf(self, conf):
        """
        Set details for robot conf
        """

        url = f'http://{self.gmu_ip}:{self.gmu_port}/robot/config'
        response = requests.post(url, json=conf, timeout=1)
        response.raise_for_status()
        response_body = response.content.decode('utf-8')
        return response_body