#include "src/EventLoopThread.h"
#include "src/EventLoop.h"

EventLoopThread::EventLoopThread(thread_init_callback cb)
    : loop_(nullptr)
    , callback_(std::move(cb))
{}

EventLoopThread::~EventLoopThread()
{
    if (loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::start_loop()
{
    thread_ = std::thread(&EventLoopThread::thread_func, this);
    /**
     * 此时存在两个线程，调用start_loop()的线程和新创建的线程
     * 接下来不确定哪个线程先执行，所以需要使用条件变量等待loop不为空
     */
    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] {return this->loop_ != nullptr;});
        loop = loop_;
    }
    return loop;
}

void EventLoopThread::thread_func()
{
    EventLoop loop;
    
    if (callback_)
    {
        callback_(&loop);
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        // loop_指针指向了一个栈上的对象，thread_func函数退出之后，这个指针就失效了
        // 不过thread_func函数退出，就意味着线程退出了，EventLoopThread对象也就没有存在价值了
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();

    std::lock_guard<std::mutex> lock(mutex_);
    loop_ = nullptr;
}