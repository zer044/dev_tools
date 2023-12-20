from setcallback import RobotCallback, CallbackType
from flask import Flask, request, render_template
import base64
import cv2 as cv
import numpy as np

app = Flask(__name__)

@app.route('/robot/set-heartbeat', methods=['POST'])
def receive_heartbeat():
    data = request.data
    print(data)
    return 'OK'

if __name__ == '__main__':
    callback_server_data = {
        'IP': '192.168.1.42',
        'port': 3333,
        'url': '/robot/set-heartbeat',
        'type': CallbackType.HEARTBEAT.value
    }

    gmu_ip = '192.168.1.205'
    gmu_port = '8080'

    callback = RobotCallback(callback_server_data['IP'], callback_server_data['port'], callback_server_data['url'], callback_server_data['type'])
    response_body = callback.send_callback(gmu_ip, gmu_port)

    # Start the Flask server
    app.run(host='0.0.0.0', port=3333)
