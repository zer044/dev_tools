from setcallback import RobotCallback, CallbackType
from flask import Flask, request, render_template
import base64
import cv2 as cv
import numpy as np

app = Flask(__name__)

@app.route('/robot/set-image', methods=['POST'])
def receive_image():
    data = request.data

    # Decode and verify if the image is valid
    jpg_original = base64.b64decode(data)

    # Convert the image data to a format compatible with HTML
    jpg_as_np = np.frombuffer(jpg_original, dtype=np.uint8)
    image_buffer = cv.imdecode(jpg_as_np, flags=1)
    _, img_encoded = cv.imencode('.png', image_buffer)
    img_base64 = base64.b64encode(img_encoded).decode('utf-8')

    # Save the image to a file
    with open('image.png', 'wb') as f:
        f.write(jpg_original)

    # Render the template with the image
    return render_template('image.html', img_base64=img_base64)


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

    # Start the Flask server
    app.run(host='0.0.0.0', port=3333)
