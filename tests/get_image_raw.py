import base64
import cv2 as cv
import numpy as np
import GPAPI


if __name__ == '__main__':
    # Create an instance of the GPAPI class
    box_ip = "192.168.0.187"
    box_port = 8080
    gmu = GPAPI.GPAPI(box_ip, box_port)

    image = gmu.get_image_raw()
    # Decode the base64 image to a byte array
    image_bytes = base64.b64decode(image)
    image_np = np.frombuffer(image_bytes, dtype=np.uint8)

    # Convert the byte array to an OpenCV image
    image = cv.imdecode(image_np, cv.IMREAD_COLOR)

    # resize image to 640x480
    image_sized = cv.resize(image, (640, 480))

    # Display the image
    cv.imshow('image', image_sized)
    # ESC as the wait key
    esc_key = 27
    if cv.waitKey(0) == esc_key:
        cv.destroyAllWindows()

