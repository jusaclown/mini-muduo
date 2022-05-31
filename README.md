## v0.1

这是一个echo服务器，只是将客户端发来的消息返回给他  

看完《Linux高性能网络编程》应该很容易就能理解

## v0.2
将上一个版本简单的封装成TcpServer

## v0.3
加入Channel  
Channel对象负责文件描述符的IO事件分发，它不拥有文件描述符。类似pollfd结构体，它也有events_和revents_两个成员，分别表示用户关注的事件和epoll 返回的事件。  
通过在Channel类中设置对应事件的回调函数，我们可以控制当文件描述符fd发生可读/可写等事件时，如何处理这些事件。  

目前Channel有两类：
* 监听套接字对应的Channel，当其可读时，说明一个新连接到来，此时通过`handle_listenfd_()`函数来`accept4()`新连接。
* 客户端连接对应的Channel，当其可读时，说明有数据到来或连接关闭，使用`handle_clientfd_(int fd)`接收数据。

## v0.4
在v0.3中，TcpServer既要处理监听套接字，又要处理客户端连接，在此版本中，我们加入Acceptor和TcpConnection，将上述两个分别封装至对应的类。  
Acceptor用于accept新TCP连接，并通过回调通知使用者，

![](./picture/listen%E5%8F%AF%E8%AF%BB.jpg)

> 框架负责从套接字读取数据给用户或者把用户的数据通过套接字发送出去