#send http command to update speed periodically

import requests
import time

url = 'http://192.168.1.186:8080/robot/set-speed'
speed = 0.44
myobj = {'v': speed}

counter = 0

while(True):
    speed = 0.44
    myobj = {'v': speed}
    #delay 1 second
    time.sleep(1)


