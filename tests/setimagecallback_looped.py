import time
from setcallback import RobotCallback, CallbackType

while True:
    if __name__ == '__main__':
        callback_server_data = {
            'IP': '10.9.0.56',
            'port': 3333,
            'url': '/robot/set-image',
            'type': CallbackType.IMAGE.value
        }

    gmu_ip = '10.9.0.197'
    gmu_port = '8080'

    callback = RobotCallback(callback_server_data['IP'], callback_server_data['port'], callback_server_data['url'], callback_server_data['type'])
    response_body = callback.send_callback(gmu_ip, gmu_port)

    # delay 0.1 seconds
    time.sleep(0.1)