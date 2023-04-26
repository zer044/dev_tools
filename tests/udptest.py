import socket
import json

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
message = {"foo": "bar"}
message = json.dumps(message).encode('utf-8') + b'\n'
sock.sendto(message, ('localhost', 54321))
