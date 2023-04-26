import requests
import json

# Define base URL string that can be set once
base_url = "http://192.168.1.205:8080"  # Replace with actual IP address or hostname

# Define endpoints relative to base URL
#read enpoints from file "endpoints.json"
endpoints = []
with open("endpoints.json", "r") as f:
    endpoints = json.load(f)


# Construct full URLs and make requests
for endpoint in endpoints:
    if endpoint["url"] != "/robot/config":
        continue
    url = base_url + endpoint["url"]
    if endpoint["method"] == "POST":
        response = requests.post(url, headers=endpoint["headers"], json=endpoint["data"])
    elif endpoint["method"] == "GET":
        response = requests.get(url, params=endpoint.get("params"))
        # print(response.text)

    if response.status_code >= 200 and response.status_code < 300:
        print(f"Endpoint {url} passed")
    else:
        print(f"Endpoint {url} failed with status code {response.status_code}")
