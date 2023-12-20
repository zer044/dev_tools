import paramiko
import socket

def is_host_reachable(ip):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(0.2)
    try:
        s.connect((ip, 22))
        s.shutdown(2)
        return True
    except:
        return False

def get_hostname(ip, username, password):
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    try:
        ssh.connect(ip, username=username, password=password)
        stdin, stdout, stderr = ssh.exec_command("hostname")
        hostname = stdout.read().decode().strip()
        ssh.close()
        return hostname
    except:
        return None

with open("hostnames.txt", "w") as file:
    for i in range(1, 255):
        ip = "10.9.0." + str(i)
        if not is_host_reachable(ip):
            continue
        print(f"Scanning {ip}")
        hostname = get_hostname(ip, "elroy", "robotics")
        if hostname:
            print(f"Found {hostname}")
            file.write(f"{ip}, {hostname}\n")
