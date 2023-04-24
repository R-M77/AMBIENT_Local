
import picamera
import time
import os
import subprocess
import sys


cam1 = "10.42.0.57"
cam2 = "10.42.0.197"
video_file_cam11 = ""
video_file_cam12 = ""
combined_cam1 = ""
video_file_cam21 = ""
video_file_cam22 = ""
combined_cam2 = ""

def start_recording(tagID):
    global video_file_cam11
    global combined_cam1
    global video_file_cam21
    global combined_cam2
    video_file_cam11 = f"{tagID}_cam1_{time.strftime('%Y%m%d-%H%M%S')}"
    video_file_cam21 = f"{tagID}_cam2_{time.strftime('%Y%m%d-%H%M%S')}"

    with picamera.PiCamera() as camera1:
        camera1.resolution = (1280,720)
        camera1.framerate = 30
        camera1.start_recording(combined_cam1, format='h264')
    
    with picamera.PiCamera() as camera2:
        camera2.resolution = (1280,720)
        camera2.framerate = 30
        camera2.start_recording(combined_cam2, format='h264')


def stop_recording():
    with picamera.PiCamera() as camera1:
        camera1.stop_recording()

    with picamera.PiCamera() as camera2:
        camera2.stop_recording()
    
    # call function to split video file if overlapped
    start1 = 0
    start2 = 10 #get from main cpp
    end1 = 50 #get from main cpp
    end2 = 60 #get from main cpp

    split_video(start1, start2, end1, end2)

    remote_path = '/home/rm/'
    send_file(video_file_cam11, remote_path)
    send_file(video_file_cam21, remote_path)


def split_video(tag1_start, tag2_start, tag1_end, tag2_end):
    tag1_duration = tag1_end - tag1_start
    tag2_duration = tag2_end - tag2_start
    
    # Split the combined video into two separate videos
    command = f"ffmpeg -i {combined_cam1} -vcodec copy -acodec copy -ss 00:00:{tag1_start:02d} -t 00:00:{tag1_duration:02d} {video_file_cam11}"
    os.system(command)

    command = f"ffmpeg -i {combined_cam2} -vcodec copy -acodec copy -ss 00:00:{tag2_start:02d} -t 00:00:{tag2_duration:02d} {video_file_cam11}"
    os.system(command)

def send_file(file, remote_path):
    remote_address = 'rm@10.42.0.1:{}'.format(remote_path)
    subprocess.call(['scp', '-o', 'StrictHostKeyChecking=no', '-o', 'UserKnownHostsFile=/dev/null', file, remote_address])

    # check if files sent successfully
    # *********
    # CHECK SSH
    # *********

    os.remove(video_file_cam11)
    os.remove(video_file_cam21)

def main(action, tagID=None):
    if action == 'start' and tagID is not None:
        start_recording(tagID)
    elif action == 'stop':
        stop_recording()
    else:
        print("Invalid arguments passed")



# def record_video(start_time, duration, filename):
#     with picamera.PiCamera() as camera:
#         camera.resolution = (1280, 720)
#         camera.framerate = 30
#         time.sleep(start_time)
#         camera.start_recording(filename, format='h264')
#         camera.wait_recording(duration)
#         camera.stop_recording()


if __name__ == '__main__':
    if len(sys.argv) > 2:
        main(sys.argv[1], sys.argv[2])
    elif len(sys.argv) > 1:
        main(sys.argv[1])
    else:
        print("No arguments passed")


# if __name__ == '__main__':
#     tag1_start_time = 2
#     tag1_duration = 40
#     tag2_start_time = 10
#     tag2_duration = 20

#     record_video(0, tag1_start_time + tag1_duration, 'combined_video.h264')

#     # Split the combined video into two separate videos
#     command = "ffmpeg -i combined_video.h264 -vcodec copy -acodec copy -ss 00:00:{:02d} -t 00:00:{:02d} video1.h264".format(tag1_start_time, tag1_duration)
#     os.system(command)

#     command = "ffmpeg -i combined_video.h264 -vcodec copy -acodec copy -ss 00:00:{:02d} -t 00:00:{:02d} video2.h264".format(tag2_start_time, tag2_duration)
#     os.system(command)

#     print('Both videos created successfully!')

#     # Send files to remote Linux PC
#     remote_path = '/home/rm/'
#     send_file('video1.h264', remote_path)
#     send_file('video2.h264', remote_path)

#     # Delete the combined video, video1, and video2
#     os.remove('combined_video.h264')
#     os.remove('video1.h264')
#     os.remove('video2.h264')

#     print('Files sent and deleted successfully!')


# import sys
# import picamera
# import time
# import os
# import subprocess
# import datetime
# from multiprocessing import Process

# def record_video(tagID, filename):
#     with picamera.PiCamera() as camera:
#         camera.resolution = (1280, 720)
#         camera.framerate = 30
#         camera.start_recording(filename, format='h264')
#         while True:
#             camera.wait_recording(1)

# def send_file(file, remote_path):
#     remote_address = 'rm@10.42.0.1:{}'.format(remote_path)
#     subprocess.call(['scp', '-o', 'StrictHostKeyChecking=no', '-o', 'UserKnownHostsFile=/dev/null', file, remote_address])

# if __name__ == '__main__':
#     if len(sys.argv) < 2:
#         print("Please provide a command: start, stop")
#         sys.exit(1)

#     command = sys.argv[1]

#     if command == "start":
#         if len(sys.argv) < 3:
#             print("Please provide a tag ID")
#             sys.exit(1)

#         tagID = sys.argv[2]
#         timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
#         filename = "cam{}_{}_{}.h264".format(1, tagID, timestamp)  # Change cam{}_ to cam1_ or cam2_ based on the camera being used

#         # Record the video for the specified tag ID
#         recording_process = Process(target=record_video, args=(tagID, filename))
#         recording_process.start()

#     elif command == "stop":
#         if recording_process.is_alive():
#             recording_process.terminate()
#             recording_process.join()

#             # Send the video file to the remote Linux PC
#             remote_path = '/home/rm/'
#             send_file(filename, remote_path)

#             # Delete the video file
#             os.remove(filename)
#     else:
#         print("Invalid command. Please use 'start' or 'stop'.")
