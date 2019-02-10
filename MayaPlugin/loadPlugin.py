import sys
import os
import socket

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('127.0.0.1', 1234))

# ask Maya for its PID :) 
s.send("getpid\n")
answer = s.recv(1024) 
answer = answer[0:answer.find('\n')]

# attach maya process (dialog pops up)
os.system('vsjitdebugger -p ' + answer)

# new file
s.send('file -f -new\n')
answer = s.recv(1024)

# load plugin
#s.send('loadPlugin("C:/Users/BTH/Desktop/MayaPlugin/x64/Debug/MayaAPI.mll")\n')
s.send('loadPlugin("C:/Users/Mudzi/Desktop/API3/MayaPlugin/x64/Debug/MayaAPI.mll")\n')
answer = s.recv(1024)

# create a polycube
s.send('polyPlane -sy 1 -sx 1\n')
#s.send('polyCube')
s.send('polyTriangulate')
answer = s.recv(1024)

# close socket with Maya
s.close()
