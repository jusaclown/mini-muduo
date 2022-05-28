import socket

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("localhost", 8888))

msg = "lalalala demaxiya"
s.send(msg.encode("utf-8"))
print("recv: ", s.recv(1024).decode())

s.close()