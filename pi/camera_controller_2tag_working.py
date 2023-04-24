import time
import os
import sys
import subprocess

tag_start_times = {}
tag_durations = {}  # Add a dictionary to keep track of the recording duration for each tag
# create folder with %Y%m%d and set as remote_path

remote_path = f"rm@10.42.0.1:/home/rm/{time.strftime('%Y%m%d')}"
camera_pi1 = "10.42.0.57"
camera_pi2 = "10.42.0.197"

def start_recording(tagID): #DONE
    # global camera_pi1
    # global camera_pi2 
    global video_file
    # camera_pi1 = "10.42.0.57"
    # camera_pi2 = "10.42.0.197"

    video_file = f"{tagID}_{time.strftime('%Y%m%d-%H%M%S')}"
    cmd1 = f"raspivid -o {video_file}_cam1.h264 -t 40000"
    cmd2 = f"raspivid -o {video_file}_cam2.h264 -t 40000"

    subprocess.Popen(["ssh", f"pi@{camera_pi1}", cmd1])
    subprocess.Popen(["ssh", f"pi@{camera_pi2}", cmd2])

def stop_recording(): #DONE
    cmd = "pkill raspivid"
    for ip in [camera_pi1, camera_pi2]:
        subprocess.Popen(["ssh", f"pi@{ip}", cmd])   
      

def split_video(source_filename, start_time, duration, destination_filename): #DONE
    command = ['ffmpeg', '-i', source_filename, '-ss', '00:00:' + str(start_time), '-t', '00:00:' + str(duration), '-codec:v', 'copy', '-codec:a', 'copy', '-f', 'h264', destination_filename]
    subprocess.call(command)

def send_file(file): #DONE
    subprocess.call(['scp', '-o', 'StrictHostKeyChecking=no', '-o', 'UserKnownHostsFile=/dev/null', file, remote_path])

def remove_file(file): 
    os.remove(file)

def main(action, tagID=None, tag2_start=None, tag1_duration=None, tag2_duration=None):
    if action == "split" and tagID is not None and tag2_start is not None and tag1_duration is not None and tag2_duration is not None:
        source_filename1 = video_file + "_cam1.h264"
        # source_filename2 = video_file + "_cam2.h264"
        source_filename2 = f"{tagID}_{time.strftime('%Y%m%d-%H%M%S')}"
        global destination_filename1_1, destination_filename1_2, destination_filename2_1, destination_filename2_2
        destination_filename1_1 = video_file + "_cam1_tag1.h264"
        destination_filename1_2 = video_file + "_cam1_tag2.h264"
        destination_filename2_1 = video_file + "_cam2_tag1.h264"
        destination_filename2_2 = video_file + "_cam2_tag2.h264"
        split_video(source_filename1, 0, tag1_duration, destination_filename1_1)
        split_video(source_filename1, tag2_start, tag2_duration, destination_filename1_2)
        split_video(source_filename2, 0, tag1_duration, destination_filename2_1)
        split_video(source_filename2, tag2_start, tag2_duration, destination_filename2_2)
        print('Videos split successfully!')
    elif action == "start" and tagID is not None:
        start_recording(tagID)
        print('Recording started!')
    elif action == "stop":
        stop_recording()
        print('Recording stopped!')
    elif action == "send":
        send_file(destination_filename1_1)
        send_file(destination_filename1_2)
        send_file(destination_filename2_1)
        send_file(destination_filename2_2)
        # check if files are sent
        # if yes, remove files
        # remove_file(destination_filename1_1)
        # remove_file(destination_filename1_2)
        # remove_file(destination_filename2_1)
        # remove_file(destination_filename2_2)
        print('Files sent and deleted successfully!')
    else:
        print("Invalid action. Please use 'start' or 'stop'.")

if __name__ == '__main__':
    if len(sys.argv) > 3:
        main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5])
    elif len(sys.argv) > 2:
        main(sys.argv[1], sys.argv[2])
    elif len(sys.argv) > 1:
        main(sys.argv[1])
    else:
        print("Please provide an action: 'start' or 'stop'.")



# import time
# import os
# import sys
# import subprocess

# camera_pi1 = "10.42.0.57"
# camera_pi2 = "10.42.0.197"

# tag_start_times = {}
# tag_durations = {}  # Add a dictionary to keep track of the recording duration for each tag

# def record_video(start_time, duration, filename):
#     command = ['ffmpeg', '-f', 'video4linux2', '-s', '1280x720', '-i', '/dev/video0', '-ss', str(start_time), '-t', str(duration), '-codec:v', 'libx264', '-pix_fmt', 'yuv420p', '-preset', 'ultrafast', '-b:v', '5000k', '-f', 'h264', filename]
#     subprocess.call(command)

# def split_video(source_filename, start_time, duration, destination_filename):
#     command = ['ffmpeg', '-i', source_filename, '-ss', '00:00:' + str(start_time), '-t', '00:00:' + str(duration), '-codec:v', 'copy', '-codec:a', 'copy', '-f', 'h264', destination_filename]
#     subprocess.call(command)

# def send_file(file, remote_path):
#     remote_address = 'rm@10.42.0.1:{}'.format(remote_path)
#     subprocess.call(['scp', '-o', 'StrictHostKeyChecking=no', '-o', 'UserKnownHostsFile=/dev/null', file, remote_address])

# if __name__ == '__main__':
#     # should get these from reader program
#     tag1_start_time = ?
#     tag1_duration = ?
#     tag2_start_time = ?
#     tag2_duration = ?

#     record_video(0, tag1_start_time + tag1_duration, 'combined_video.h264')

#     # Split the combined video into two separate videos
#     split_video('combined_video.h264', tag1_start_time, tag1_duration, 'video1.h264')
#     split_video('combined_video.h264', tag2_start_time, tag2_duration, 'video2.h264')

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


# import time
# import os
# import sys
# import subprocess

# camera_pi1 = "10.42.0.57"
# camera_pi2 = "10.42.0.197"

# tag_start_times = {}
# tag_durations = {}  # Add a dictionary to keep track of the recording duration for each tag

# def start_recording(tagID):
#     global tag_start_times
#     tag_start_times[tagID] = time.time()
#     video_file = f"{tagID}_{time.strftime('%Y%m%d-%H%M%S')}"
#     cmd1 = f"raspivid -o {video_file}_cam1.h264 -t 40000"
#     cmd2 = f"raspivid -o {video_file}_cam2.h264 -t 40000"

#     subprocess.Popen(["ssh", f"pi@{camera_pi1}", cmd1])
#     subprocess.Popen(["ssh", f"pi@{camera_pi2}", cmd2])

# def stop_recording(tagID):
#     global tag_start_times, tag_durations
#     duration = time.time() - tag_start_times[tagID]
#     tag_durations[tagID] = duration
#     cmd = "pkill raspivid"
#     for ip in [camera_pi1, camera_pi2]:
#         subprocess.Popen(["ssh", f"pi@{ip}", cmd])

# def main(action, tagID=None):
#     global tag_start_times, tag_durations
    
#     if action == "start" and tagID is not None:
#         if tagID not in tag_start_times:
#             tag_start_times[tagID] = time.time()
#             start_recording(tagID)
#         elif tagID in tag_durations and tag_durations[tagID] < 4:
#             print(f"Tag {tagID} has been scanned too soon after the previous scan.")
#         else:
#             tag_start_times[tagID] = time.time()
#             start_recording(tagID)
#     elif action == "stop" and tagID is not None:
#         stop_recording(tagID)
#         # send_to_PC()
#     else:
#         print("Invalid action. Please use 'start' or 'stop'.")

# if __name__ == "__main__":
#     if len(sys.argv) > 2:
#         main(sys.argv[1], sys.argv[2])
#     elif len(sys.argv) > 1:
#         main(sys.argv[1])
#     else:
#         print("Please provide an action: 'start' or 'stop'.")



# # APRIL 17

# import time
# import os
# import sys
# import subprocess

# camera_pi1 = "10.42.0.57"
# camera_pi2 = "10.42.0.197"

# tag_start_times = {}
# tag_scan_count = {}  # Add a dictionary to keep track of the scan count for each tag

# def start_recording(tagID):
#     global tag_start_times
#     tag_start_times[tagID] = time.time()
#     video_file = f"{tagID}_{time.strftime('%Y%m%d-%H%M%S')}"
#     cmd1 = f"raspivid -o {video_file}_cam1.h264 -t 40000"
#     cmd2 = f"raspivid -o {video_file}_cam2.h264 -t 40000"

#     subprocess.Popen(["ssh", f"pi@{camera_pi1}", cmd1])
#     subprocess.Popen(["ssh", f"pi@{camera_pi2}", cmd2])

# def stop_recording(tagID):
#     cmd = "pkill raspivid"
#     for ip in [camera_pi1, camera_pi2]:
#         subprocess.Popen(["ssh", f"pi@{ip}", cmd])

# def main(action, tagID=None):
#     global tag_scan_count
    
#     if action == "start" and tagID is not None:
#         if tagID not in tag_scan_count:
#             tag_scan_count[tagID] = 1
#             start_recording(tagID)
#         elif tag_scan_count[tagID] == 1:
#             tag_scan_count[tagID] += 1
#             stop_recording(tagID)
#             # Reset the count for the tag
#             tag_scan_count[tagID] = 0
#         else:
#             print(f"Unexpected scan count for tag {tagID}: {tag_scan_count[tagID]}")
#     elif action == "stop" and tagID is not None:
#         stop_recording(tagID)
#         # send_to_PC()
#     else:
#         print("Invalid action. Please use 'start' or 'stop'.")

# if __name__ == "__main__":
#     if len(sys.argv) > 2:
#         main(sys.argv[1], sys.argv[2])
#     elif len(sys.argv) > 1:
#         main(sys.argv[1])
#     else:
#         print("Please provide an action: 'start' or 'stop'.")




# APRIL 14
# import time
# import os
# import sys
# import subprocess

# def record_video(start_time, duration, filename):
#     command = ['ffmpeg', '-f', 'video4linux2', '-s', '1280x720', '-i', '/dev/video0', '-ss', str(start_time), '-t', str(duration), '-codec:v', 'libx264', '-pix_fmt', 'yuv420p', '-preset', 'ultrafast', '-b:v', '5000k', '-f', 'h264', filename]
#     subprocess.call(command)

# def split_video(source_filename, start_time, duration, destination_filename):
#     command = ['ffmpeg', '-i', source_filename, '-ss', '00:00:' + str(start_time), '-t', '00:00:' + str(duration), '-codec:v', 'copy', '-codec:a', 'copy', '-f', 'h264', destination_filename]
#     subprocess.call(command)

# def send_file(file, remote_path):
#     remote_address = 'rm@10.42.0.1:{}'.format(remote_path)
#     subprocess.call(['scp', '-o', 'StrictHostKeyChecking=no', '-o', 'UserKnownHostsFile=/dev/null', file, remote_address])

# camera_pi1 = "10.42.0.57"
# camera_pi2 = "10.42.0.197"
# video_file = ""

# timestamps = {}

# def start_recording(tagID):
#     global video_file
#     if tagID in timestamps:
#         print(f"Recording already started for tag {tagID}")
#         return
    
#     timestamps[tagID] = {"start": time.time(), "end": None}
#     video_file = f"{tagID}_{time.strftime('%Y%m%d-%H%M%S')}"

#     # Start recording video using record_video method
#     record_video(0, 40, f"{video_file}_cam1.h264")
#     record_video(0, 40, f"{video_file}_cam2.h264")

#     print(f"Recording started for tag {tagID}")


# def stop_recording(tagID):
#     global video_file, timestamps
#     if tagID not in timestamps:
#         print(f"No recording started for tag {tagID}")
#         tag1 = tagID
#         return
#     tag2 = tagID
#     timestamps[tag1]["count"] += 1
#     timestamps[tag2]["count"] += 1

#     # Check if both tags have been read twice or if tag1 has been read twice and tag2 has not been read yet
#     if (all(times["count"] == 2 for times in timestamps.values()) or 
#         (timestamps.get(tag1, {}).get("count", 0) == 2 and timestamps.get(tag2, {}).get("count", 0) == 0)):
#         # Both tags have been read twice, stop recording
#         cmd = "pkill ffmpeg"
#         for ip in [camera_pi1, camera_pi2]:
#             subprocess.Popen(["ssh", f"pi@{ip}", cmd])

#         # Split the combined video into two separate videos
#         for cam in ["cam1", "cam2"]:
#             source_file = f"{video_file}_{cam}.h264"
#             for tag, times in timestamps.items():
#                 start_time = times["start"] - timestamps[tagID]["start"]
#                 end_time = times["end"] - timestamps[tagID]["start"]
#                 duration = end_time - start_time
#                 destination_file = f"{tag}_{cam}.h264"
#                 split_video(source_file, start_time, duration, destination_file)

#         # Send files to remote Linux PC
#         remote_path = '/home/rm/'
#         for tag in timestamps:
#             for cam in ["cam1", "cam2"]:
#                 send_file(f"{tag}_{cam}.h264", remote_path)

#         # Delete the video files
#         os.remove(f"{video_file}_cam1.h264")
#         os.remove(f"{video_file}_cam2.h264")

#         # Reset timestamps
#         timestamps = {}

#         print("Recording stopped for both tags")
#     else:
#         # Only one tag has been read twice
#         print(f"Tag {tagID} read for the second time")


# def main(action, tagID):
#     if action == "start" and tagID is not None:
#         start_recording(tagID)
#     elif action == "stop" and tagID is not None:
#         stop_recording(tagID)
#     else:
#         print("Invalid action. Please use 'start' or 'stop'.")

# if __name__ == "__main__":
#     if len(sys.argv) > 2:
#         main(sys.argv[1], sys.argv[2])
#     # elif len(sys.argv) > 1:
#     #     main(sys.argv[1])
#     else:
#         print("Please provide an action: 'start' or 'stop'.")






# import picamera
# import time
# import os
# import subprocess

# def record_video(start_time, duration, filename):
#     with picamera.PiCamera() as camera:
#         camera.resolution = (1280, 720)
#         camera.framerate = 30
#         time.sleep(start_time)
#         camera.start_recording(filename, format='h264')
#         camera.wait_recording(duration)
#         camera.stop_recording()

# def send_file(file, remote_path):
#     remote_address = 'rm@10.42.0.1:{}'.format(remote_path)
#     subprocess.call(['scp', '-o', 'StrictHostKeyChecking=no', '-o', 'UserKnownHostsFile=/dev/null', file, remote_address])



# if __name__ == '__main__':
#     tag1_start_time = 2
#     tag1_duration = 25
#     tag2_start_time = 5
#     tag2_duration = 10

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
