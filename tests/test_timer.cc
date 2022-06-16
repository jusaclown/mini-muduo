#include "src/common.h"
#include "src/EventLoop.h"

#include <string>
#include <iostream>
#include <unistd.h>

using std::chrono::milliseconds;

int count = 0;
EventLoop* g_loop;

void print(const std::string& msg)
{
    static auto time = timer_clock::now().time_since_epoch().count();
    auto now = timer_clock::now().time_since_epoch().count();
    std::cout << now << " " << msg << ", 距离上一次 " << (now - time)/1000000000.0 << std::endl;
    time = now;
    if (++count == 20)
        g_loop->quit();
}

void cancel(TimerId timer)
{
    g_loop->cancel(timer);
    std::cout << "calcel at " << timer_clock::now().time_since_epoch().count() << std::endl;
}

TimerId to_cancel;
void cancel_self()
{
    std::cout << "cancel self" << std::endl;
    g_loop->cancel(to_cancel);
}

int main()
{
    std::cout << timer_clock::now().time_since_epoch().count() << std::endl;
    sleep(1);
    
    EventLoop loop;
    g_loop = &loop;
    std::cout << "main" << std::endl;
    loop.run_after(milliseconds(1000), std::bind(print, "run after 1s"));
    loop.run_after(milliseconds(1500), std::bind(print, "run after 1.5s"));
    loop.run_after(milliseconds(2500), std::bind(print, "run after 2.5s"));
    loop.run_after(milliseconds(3500), std::bind(print, "run after 3.5s"));
    TimerId t45 = loop.run_after(milliseconds(4500), std::bind(print, "run after 4.5"));
    loop.run_after(milliseconds(4200), std::bind(cancel, t45));
    loop.run_after(milliseconds(4800), std::bind(cancel, t45));
    loop.run_every(milliseconds(2000), std::bind(print, "run every 2s"));
    TimerId te3 = loop.run_every(milliseconds(3000), std::bind(print, "run every 3s"));
    loop.run_after(milliseconds(9100), std::bind(cancel, te3));
    to_cancel = loop.run_every(milliseconds(5000), cancel_self);

    loop.loop();
    print("main loop exits");

    return 0;
}
