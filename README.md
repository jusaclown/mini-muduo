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
* Acceptor用于创建监听套接字并accept新TCP连接，新连接到来之后则交由回调函数处理。
* TcpConnection则表示一个连接对象。

![](./picture/listen%E5%8F%AF%E8%AF%BB.jpg)

## v0.5
加入EventLoop和Epoll，for循环和IO multiplexing分别被包装起来  
此前，TcpServer一直充当着EventLoop的角色
* EpollPoller 创建epoll对象，epoll_wait活动fd，将fd转成Channel返回给调用对象。

## v0.6
包装成库，库与库的用户分离

## v0.7
加入发送和接收缓冲区  
Q: 什么时候下会用到发送缓冲区？

## V0.8 
加入计时器
使用小根堆实现定时器  
在EchoServer中踢掉长时间未活动的连接  

> 框架负责从套接字读取数据给用户或者把用户的数据通过套接字发送出去