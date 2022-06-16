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
            msg = s.recv(66560).decode()
            recv_len += len(msg)
            print("{} recv: '{}', len = {}, total_len = {}\n".format(self.name, msg, len(msg), recv_len))
            n -= len(msg)

        time.sleep(self.delay)
        if (self.name == "Thread-3"):
            s.send(self.msg.encode("utf-8"))
            time.sleep(self.delay*2)
        s.close()
        print(self.name, " is over")

# 创建新线程
thread1 = myThread("Thread-1", 2, "lalalala demaxiya "*10)      # 2s close
thread2 = myThread("Thread-2", 10, "lalalala lulalula "*10)     # 5s shutdown 10s close
thread3 = myThread("Thread-3", 3, "lalalala lulalula "*10)      # 8s shutdown 9s close

# 开启新线程
thread1.start()
thread2.start()
thread3.start()

# 等待所有线程完成
thread1.join()
thread2.join()
thread3.join()

print ("退出主线程")