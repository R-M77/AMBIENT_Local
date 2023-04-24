import picamera
import time
import os
import subprocess
import sys

recordings = {}

def record_video(tagID, duration, filename):
    with picamera.PiCamera() as camera:
        camera.resolution = (1280, 720)
        camera.framerate = 30
        camera.start_recording(filename, format='h264')
        camera.wait_recording(duration)
        camera.stop_recording()

def send_file(file, remote_path):
    remote_address = 'rm@10.42.0.1:{}'.format(remote_path)
    subprocess.call(['scp', '-o', 'StrictHostKeyChecking=no', '-o', 'UserKnownHostsFile=/dev/null', file, remote_address])

def start_recording(tagID):
    recordings[tagID] = {
        "start_time": time.time(),
        "video_file": f"{tagID}_{time.strftime('%Y%m%d-%H%M%S')}.h264"
    }
    record_video(tagID, None, recordings[tagID]["video_file"])

def stop_recording(tagID):
    if tagID in recordings:
        duration = time.time() - recordings[tagID]["start_time"]
        record_video(tagID, duration, recordings[tagID]["video_file"])
        send_file(recordings[tagID]["video_file"], '/home/rm/')
        os.remove(recordings[tagID]["video_file"])
        del recordings[tagID]

def main(action, tagID=None):
    if action == "start" and tagID is not None:
        start_recording(tagID)
    elif action == "stop" and tagID is not None:
        stop_recording(tagID)
    else:
        print("Invalid action. Please use 'start' or 'stop'.")

# if __name__ == "__main__":
#     if len(sys.argv) > 2:
#         main(sys.argv[1], sys.argv[2])
#     else:
#         print("Please provide an action: 'start' or 'stop'.")

if __name__ == "__main__":
    if len(sys.argv) > 2:
        main(sys.argv[1], sys.argv[2])
    elif len(sys.argv) > 1:
        main(sys.argv[1])
    else:
        print("Please provide an action: 'start' or 'stop'.")
