import requests


results = []
for i in range(1,245):
    try:
        url = "http://10.9.0." + str(i) + ":8080/get_box_identifier"
        print("Trying: " + url)
        r= requests.get(url, verify=False, timeout=2)
        if "BOX_IDENTIFIER" in r.text: 
            results.append(str(i) + " " + r.text)
            print(r.text)
    except:
        continue

#Write results to file
with open('results.txt', 'w') as f:
    for item in results:
        f.write("10.9.0." + item + "\n")
f.close()
