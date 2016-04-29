import os
import time
import sys

server_name= sys.argv[1]
ip_address= sys.argv[2]
server_port= sys.argv[3]
file_name= sys.argv[4]
output_file= "output.txt"

name= server_name.split("-")
suffix= name[0][-2:]

client_name= "client"+suffix

output= open(output_file, "a")

server_launch_command= "./" +server_name +" " +server_port +" &"
client_launch_command= "./" +client_name +" " +ip_address +" " +server_port +" " +file_name + " 0"

os.system(server_launch_command)
time.sleep(1)
start_time= time.time()
os.system(client_launch_command)
end_time= time.time()
delta= end_time- start_time

copy_file_name= "./copy_" +file_name
statinfo= os.stat(copy_file_name)

time.sleep(1)

output.write(str(server_name)+" "+ str(statinfo.st_size)+ " "+ str(delta)+ " "+ str(statinfo.st_size/delta)+ "\n")

output.close()

remove_command= "rm "+ copy_file_name
os.system(remove_command)



