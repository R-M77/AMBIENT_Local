# # import picamera
# import time
# import os
# import sys
# import subprocess

# # camera = picamera.PiCamera()
# # camera.resolution = (1280, 720)
# camera_pi1 = "10.42.0.57"
# camera_pi2 = "10.42.0.197"
# video_file = ""

# def start_recording(tagID):
#     global video_file
#     video_file = f"{tagID}_{time.strftime('%Y%m%d-%H%M%S_AE')}.h264"
#     cmd = f"raspivid -o {video_file} -t 30000"
#     for ip in [camera_pi1, camera_pi2]:
#         subprocess.Popen(["ssh", f"pi@{ip}", cmd])
#     # camera.start_recording(video_file)
#     # camera.wait_recording(40)

# def stop_recording():
#     cmd = "pkill raspivid"
#     for ip in [camera_pi1, camera_pi2]:
#         subprocess.Popen(["ssh", f"pi@{ip}", cmd])

# def send_to_PC():
#     global video_file
#     pc_username = "rm"
#     pc_ip = "10.42.0.1"
#     date_folder = time.strftime("%Y%m%d")
#     destination_path = f"/home/rm/{date_folder}"

#     # Create the date folder on the PC if it does not exist
#     folder_creation_cmd = f"ssh {pc_username}@{pc_ip} 'mkdir -p {destination_path}'"
#     try:
#         subprocess.check_output(folder_creation_cmd, shell=True)
#     except subprocess.CalledProcessError as e:
#         print(f"Error creating date folder on PC: {e}")

#     # Send the video file to the PC
#     command = f"scp {video_file} {pc_username}@{pc_ip}:{destination_path}"
#     try:
#         subprocess.check_output(command, shell=True)
#         print(f"File {video_file} sent to PC successfully.")
#     except subprocess.CalledProcessError as e:
#         print(f"Error sending file {video_file} to PC: {e}")

# def main(action, tagID=None):
#     if action == "start" and tagID is not None:
#         start_recording(tagID)
#     elif action == "stop":
#         stop_recording()
#         send_to_PC()
#     else:
#         print("Invalid action. Please use 'start' or 'stop'.")

# if __name__ == "__main__":
#     if len(sys.argv) > 2:
#         main(sys.argv[1], sys.argv[2])
#     elif len(sys.argv) > 1:
#         main(sys.argv[1])
#     else:
#         print("Please provide an action: 'start' or 'stop'.")

# import picamera
import time
import os
import sys
import subprocess
import threading

# camera = picamera.PiCamera()
# camera.resolution = (1280, 720)
camera_pi1 = "10.42.0.57"
camera_pi2 = "10.42.0.197"
video_file = ""
active_recording_threads = {} # dictionary to keep track of active recording threads for each tag

class RecordingThread(threading.Thread):
    def __init__(self, tagID):
        threading.Thread.__init__(self)
        self.tagID = tagID
        self.video_file = f"{tagID}_{time.strftime('%Y%m%d-%H%M%S_AE')}.h264"
    
    def run(self):
        cmd = f"raspivid -o {self.video_file} -t 30000"
        for ip in [camera_pi1, camera_pi2]:
            subprocess.Popen(["ssh", f"pi@{ip}", cmd])
        active_recording_threads[self.tagID] = self
    
    def stop(self):
        cmd = "pkill raspivid"
        for ip in [camera_pi1, camera_pi2]:
            subprocess.Popen(["ssh", f"pi@{ip}", cmd])
        send_to_PC(self.tagID)
        del active_recording_threads[self.tagID]

def start_recording(tagID):
    global video_file
    if tagID not in active_recording_threads:
        recording_thread = RecordingThread(tagID)
        recording_thread.start()
    else:
        print("Video already being recorded for tag ID: ", tagID)

def stop_recording(tagID):
    if tagID in active_recording_threads:
        active_recording_threads[tagID].stop()
    else:
        print("No active recording found for tag ID: ", tagID)

def send_to_PC(tagID):
    video_file = active_recording_threads[tagID].video_file
    pc_username = "rm"
    pc_ip = "10.42.0.1"
    date_folder = time.strftime("%Y%m%d")
    destination_path = f"/home/rm/{date_folder}/{tagID}"

    # Create the date and tagID folders on the PC if they do not exist
    folder_creation_cmd = f"ssh {pc_username}@{pc_ip} 'mkdir -p {destination_path}'"
    try:
        subprocess.check_output(folder_creation_cmd, shell=True)
    except subprocess.CalledProcessError as e:
        print(f"Error creating date/tagID folders on PC: {e}")

    # Send the video file to the PC
    command = f"scp {video_file} {pc_username}@{pc_ip}:{destination_path}"
    try:
        subprocess.check_output(command, shell=True)
        print(f"File {video_file} sent to PC successfully.")
    except subprocess.CalledProcessError as e:
        print(f"Error sending file {video_file} to PC: {e}")

def main(action, tagID=None):
    if action == "start" and tagID is not None:
        start_recording(tagID)
    elif action == "stop" and tagID is not None:
        stop_recording(tagID)
    else:
        print("Invalid action or tag ID. Please use 'start' or 'stop' with a valid tag ID.")

if __name__ == "__main__":
    if len(sys.argv) > 2:
        main(sys.argv[1], sys.argv[2])
    elif len(sys.argv) > 1:
        main(sys.argv[1])
    else:
        print("Please provide an action: 'start' or 'stop'.")

