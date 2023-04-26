import socket
import json

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('0.0.0.0', 1234))

while True:
    data, addr = sock.recvfrom(1500)
    try:
        data = data.decode('utf-8')
        data = json.loads(data)
        print(data)
    except ValueError as e:
        print(f"Received data is not in valid JSON format: {e}")
    except Exception as e:
        print(f"An error occurred while processing the received data: {e}")
