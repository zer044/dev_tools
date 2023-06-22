import os
import socket
import subprocess
import time
import sys
import re

DISK_DIR="/mnt/capture"
box_id = ""

def get_box_id():
    hostname = socket.gethostname()
    pattern = r'(?:agx-|rog-?)'
    box_id = re.sub(pattern, '', hostname)
    return box_id

def set_record_param(param_value, appid):
    command = ['ros2', 'param', 'set', '/gods_global_params', f'app.{appid}.enable_data_record_mode', param_value]
    result = subprocess.run(command, capture_output=True, text=True)
    output = result.stdout.strip()
    print("Setting enable_data_record_mode to " + param_value)
    print(output)
    # search output for "successful"
    pattern = r"(?i)successful"  # (?i) enables case-insensitive matching
    matches = re.findall(pattern, output)
    if len(matches) > 0:
        return True
    else:
        return False

def start_recording():
    print(f"Creating debug_record directory ... at {DISK_DIR}")
    subprocess.run(['mkdir', '-p', os.path.join(DISK_DIR, 'debug_record')])
    set_record_param("true", app_id)

    # Wait until the first image comes in
    timeout_count = 0
    while True:
        if timeout_count > 5:
            print("Could not start recording!")
            print("Check if the camera is connected and frames are being recieved")
            print("Resetting enable_data_record_mode to false")
            set_record_param("false", app_id)
            exit(1)
        number_of_files = len(os.listdir(os.path.join(DISK_DIR, 'debug_record')))
        if number_of_files > 0:
            break
        timeout_count += 1
        time.sleep(1)


def stop_recording(app_id):
    # Create the debug_record directory if it doesn't exist
    os.makedirs(os.path.join(DISK_DIR, "debug_record"), exist_ok=True)

    # Set the enable_data_record_mode to false
    set_record_param("false", app_id)

    # Wait until recording stopped
    print("Waiting for recording to stop ...")
    prev_number_of_files = -1
    timeout_count = 0

    while True:
        files = os.listdir(os.path.join(DISK_DIR, "debug_record"))
        image_files = [file for file in files if file.endswith(".jpg")]
        number_of_files = len(image_files)
        if number_of_files == prev_number_of_files:
            break
        prev_number_of_files = number_of_files
        timeout_count += 1
        if timeout_count > 5:
            print("Timeout waiting for recording to stop")
            print("Something went wrong, recording did not stop!")
            break
        time.sleep(1)
    print("Recording stopped")

def clear_debug_record_directory():
    print("Clearing debug_record directory ...")
    # Check if the debug_record directory exists
    if os.path.exists(os.path.join(DISK_DIR, 'debug_record')):
        print("Deleting old debug_record directory ...")
        subprocess.run(['rm', '-rf', os.path.join(DISK_DIR, 'debug_record')])
    else:
        print("old debug_record directory does not exist... continuing...")

def compress_record_data(start_date):
    print("Compressing debug_record directory ...")
    compressed_file_name = f'debug_record-{box_id}-{start_date}.tar.gz'
    subprocess.run(['tar', '-czf', os.path.join(DISK_DIR, compressed_file_name), os.path.join(DISK_DIR, 'debug_record')])

    # verify that the compressed file was created
    worked = os.path.exists(os.path.join(DISK_DIR, compressed_file_name))
    if worked:
        # Check file size > 0
        file_size = os.path.getsize(os.path.join(DISK_DIR,compressed_file_name))
        if file_size == 0:
            print("Error: Compressed file size is 0")
            return False
    else:
        print("Error: Could not find compressed file")
        return False

    print(f"Compressed file: {compressed_file_name}")
    return True

def search_for_true(string):
    pattern = r"(?i)true"  # (?i) enables case-insensitive matching
    matches = re.findall(pattern, string)
    if len(matches) == 0:
        return False
    return True

def check_if_recording_is_active():
    command = ['ros2', 'param', 'get', '/gods_global_params', f'app.{app_id}.enable_data_record_mode']
    enable_data_record_mode_str = subprocess.run(command, \
                           stdout=subprocess.PIPE).stdout.decode('utf-8').strip()
    return search_for_true(enable_data_record_mode_str)

def do_record(action, app_id):
    if action == "on":
        if check_if_recording_is_active() == True:
            print(f"Recording already active for {app_id}")
            return
        clear_debug_record_directory()
        start_recording()
        print(f"Recording started for {app_id}")

    if action == "off":
        if check_if_recording_is_active() == True:
            print(f"Stopping recording for {app_id} ...")
            stop_recording(app_id)
            start_date = time.strftime("%Y-%m-%d-%H-%M-%S")
            result = compress_record_data(start_date)
            if result == False:
                print("Something went wrong, could not compress debug_record!")
                print("Check if the debug_record directory exists and is not empty")
                print("else, manually compress it and download it via ftp.")
                return
            print(f"Recording stopped. Compressed data into {DISK_DIR}/debug_record-{box_id}-{start_date}.tar.gz")
        else:
            print(f"Recording not active for {app_id}")

if __name__ == "__main__":
    if(len(sys.argv) != 3):
        print("Usage: python3 debug_record.py <on/off> <app_id>")
        exit(1)
    record_state = sys.argv[1]
    app_id = sys.argv[2]
    box_id = get_box_id()

    do_record(record_state, app_id)



