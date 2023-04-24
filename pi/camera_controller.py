import time
import os
import sys
import subprocess

camera_pi1 = "10.42.0.57"
camera_pi2 = "10.42.0.197"
video_file = ""

def start_recording(tagID):
    global video_file
    video_file = f"{tagID}_{time.strftime('%Y%m%d-%H%M%S')}"
    cmd1 = f"raspivid -o {video_file}_cam1.h264 -t 40000"
    cmd2 = f"raspivid -o {video_file}_cam2.h264 -t 40000"

    subprocess.Popen(["ssh", f"pi@{camera_pi1}", cmd1])
    subprocess.Popen(["ssh", f"pi@{camera_pi2}", cmd2])

def stop_recording():
    cmd = "pkill raspivid"
    for ip in [camera_pi1, camera_pi2]:
        subprocess.Popen(["ssh", f"pi@{ip}", cmd])

def send_to_PC():
    global video_file
    pc_username = "rm"
    pc_ip = "10.42.0.1"
    date_folder = time.strftime("%Y%m%d")
    destination_path = f"/home/rm/{date_folder}"

    # Create the date folder on the PC if it does not exist
    folder_creation_cmd = f"ssh {pc_username}@{pc_ip} 'mkdir -p {destination_path}'"
    try:
        subprocess.check_output(folder_creation_cmd, shell=True)
    except subprocess.CalledProcessError as e:
        print(f"Error creating date folder on PC: {e}")

    # Send the video files to the PC
    for cam in ["cam1", "cam2"]:
        current_video_file = f"{video_file}_{cam}.h264"
        command = f"scp {current_video_file} {pc_username}@{pc_ip}:{destination_path}"
        try:
            subprocess.check_output(command, shell=True)
            print(f"File {current_video_file} sent to PC successfully.")
        except subprocess.CalledProcessError as e:
            print(f"Error sending file {current_video_file} to PC: {e}")

def main(action, tagID=None):
    if action == "start" and tagID is not None:
        start_recording(tagID)
    elif action == "stop":
        stop_recording()
        # send_to_PC()
    else:
        print("Invalid action. Please use 'start' or 'stop'.")

if __name__ == "__main__":
    if len(sys.argv) > 2:
        main(sys.argv[1], sys.argv[2])
    elif len(sys.argv) > 1:
        main(sys.argv[1])
    else:
        print("Please provide an action: 'start' or 'stop'.")
