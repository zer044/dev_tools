import requests
import json
import threading
from http.server import BaseHTTPRequestHandler, HTTPServer

# Define the callback data
callback_server_data = {
    'IP': '192.168.1.42',
    'port': '3333',
    'url': '/robot/set-items',
    'type': 'objects'
}

# Convert the callback data to JSON string
callback_data_json = json.dumps(callback_server_data)

# Send the callback data to the GMU server
gmu_ip = '192.168.1.205'
gmu_port = '8080'
callback_url = f'http://{gmu_ip}:{gmu_port}/robot/set-callback'
response = requests.post(callback_url, data=callback_data_json, headers={'Content-Type': 'application/json'})
response.raise_for_status()
response_body = response.content.decode('utf-8')
print(response_body)


# Define the HTTP server handler
class HTTPRequestHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        print(post_data.decode('utf-8'))
        self.send_response(200)
        self.end_headers()


# Define the HTTP server thread
class HTTPServerThread(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)

    def run(self):
        server_address = ('', 3333)
        httpd = HTTPServer(server_address, HTTPRequestHandler)
        print('Starting HTTP server...')
        httpd.serve_forever()


# Start the HTTP server thread
http_server_thread = HTTPServerThread()
http_server_thread.start()

# Wait for the user to exit
try:
    while True:
        pass
except KeyboardInterrupt:
    print('Exiting gracefully...')
    http_server_thread._stop() # Stop the HTTP server thread
