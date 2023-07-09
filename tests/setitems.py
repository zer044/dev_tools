from setcallback import RobotCallback, CallbackType
from flask import Flask, request, render_template
import base64
import cv2 as cv
import numpy as np
import json

app = Flask(__name__)

min_object_size = 0.01

@app.route('/robot/set-items', methods=['POST'])
def receive_items():
    data = request.data
    print(data)
    # example b'{"bs":"WARMINGUP","count":145,"frame_count":144,"objects":[{"a":0.005,"c":0.38,"h":0.082,"id":1744,"m":"grey_board","p":[-0.052,0.027],"t":0.028,"ts":1688726361.101,"w":0.065},{"a":0.005,"c":0.053,"h":0.054,"id":1748,"m":"graphic_paper","p":[-0.019,0.044],"t":0.028,"ts":1688726361.101,"w":0.085}],"ts":1688726361.101,"v":1.0}'
    # parse as json
    json_data = json.loads(data)
    # did we detect any objects?
    if len(json_data['objects']) > 0:

    return 'OK'

if __name__ == '__main__':
    callback_server_data = {
        'IP': '192.168.1.42',
        'port': 3333,
        'url': '/robot/set-items',
        'type': CallbackType.OBJECTS.value
    }

    gmu_ip = '192.168.1.205'
    gmu_port = '8080'

    callback = RobotCallback(callback_server_data['IP'], callback_server_data['port'], callback_server_data['url'], callback_server_data['type'])
    response_body = callback.send_callback(gmu_ip, gmu_port)

    # Start the Flask server
    app.run(host='0.0.0.0', port=3333)
