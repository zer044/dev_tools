from flask import Flask, request, Response
import base64
import cv2
import numpy as np
from setcallback import RobotCallback

app = Flask(__name__)

@app.route('/robot/set-items', methods=['POST'])
def set_items():
    # Get the JSON object from the request
    json_data = request.json

    # Do something with the JSON data
    print(json_data)

    return 'JSON data received'

latest_image = None

@app.route('/set-image', methods=['POST'])
def set_image():
    global latest_image

    # Get the base64-encoded image data from the request
    encoded_image = request.data

    # Decode the image data
    jpg_original = base64.b64decode(encoded_image)

    # Convert the image data to a NumPy array
    nparr = np.frombuffer(jpg_original, np.uint8)

    # Decode the NumPy array into an OpenCV image
    latest_image = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

    return 'Image received'

def gen_frames():
    global latest_image

    while True:
        # Check if there is a latest image available
        if latest_image is not None:
            # Encode the latest image as a JPEG image
            ret, buffer = cv2.imencode('.jpg', latest_image)
            frame = buffer.tobytes()

            # Yield the frame in a multipart MIME response format
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')
                   
            # Set latest_image to None to wait for the next image
            latest_image = None

@app.route('/video_feed')
def video_feed():
    # Video streaming route. Put this in the src attribute of an img tag
    return Response(gen_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

if __name__ == '__main__':
    callback_server_data = {
        'IP': '192.168.1.42',
        'port': 3333,
        'url': '/set-image',
        'type': 'image'
    }

    gmu_ip = '192.168.1.205'
    gmu_port = '8080'

    callback = RobotCallback(callback_server_data['IP'], callback_server_data['port'], callback_server_data['url'], callback_server_data['type'])
    response_body = callback.send_callback(gmu_ip, gmu_port)
    print(response_body)

    app.run(port=callback_server_data['port'], host=callback_server_data['IP'])


