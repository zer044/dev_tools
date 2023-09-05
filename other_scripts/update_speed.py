import requests
import time



ip = "10.9.0.11"
port = "8080"
endpoint = "/robot/set-speed"

url = f"http://{ip}:{port}{endpoint}"

body = {"v":1}

#loop every 0.5 seconds
while True:
    response = requests.post(url, json=body)
    print(response.text)
    #delay 0.5 seconds
    time.sleep(0.5)
