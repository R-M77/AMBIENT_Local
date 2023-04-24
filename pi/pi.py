# ---------------------------------------------------------------
#  *** This script is run on Raspberry Pi ***
# -- This script records a video and sends it to PC using SSH --
# ---------------------------------------------------------------

# install time and subprocess from vscode terminal
# pip3 install time
# pip3 install subprocess

import time
import subprocess

# Set filename
filename = "piCmd.h264"

# connect to another pi using ssh
ssh_addr = "pi@10.42.0.57"
ssh_pass = "saman123"
# subprocess.run(["sshpass", "-p", ssh_pass, "ssh", ssh_addr])

# Set record time and record video
record_time = "10000"
subprocess.run(["raspivid", "-o", filename, "-t", record_time])

# Send video to PC using SSH
# install sshpass from vscode terminal if not installed
# provide password for PC
pc_pass = "pc123"
PC_addr = "rm@10.42.0.1:/home/rm"
subprocess.run(["sshpass", "-p", pc_pass, "scp", filename, PC_addr])



# def send_file_over_ssh(file_name, host, username, password):
#     ssh = paramiko.SSHClient()
#     ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
#     ssh.connect(host, username=username, password=password)
#     sftp = ssh.open_sftp()
#     remote_path = f"{username}@{host}:{file_name}"
    
#     try:
#         sftp.put(file_name, remote_path)
#     finally:
#         sftp.close()
#         ssh.close()