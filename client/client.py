#!/usr/bin/python3
import socket
import threading
import time

class myThread (threading.Thread):
    def __init__(self, name, delay, msg):
        threading.Thread.__init__(self)
        self.name = name
        self.delay = delay
        self.msg = msg

    def run(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(("localhost", 10086))

        s.send(self.msg.encode("utf-8"))
        n = len(self.msg)
        recv_len = 0
        while (n != 0):
            msg = s.recv(4096).decode()
            recv_len += len(msg)
            print("{} recv: '{}', len = {}, total_len = {}\n".format(self.name, msg, len(msg), recv_len))
            n -= len(msg)

        time.sleep(self.delay)
        s.close()
        print(self.name, " is over")

# 创建新线程
thread1 = myThread("Thread-1", 2, "lalalala demaxiya")
thread2 = myThread("Thread-2", 5, "lalalala demaxiya "*10000)

# 开启新线程
thread1.start()
thread2.start()

# 等待所有线程完成
thread1.join()
thread2.join()

print ("退出主线程")