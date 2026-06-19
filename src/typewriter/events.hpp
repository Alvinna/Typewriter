#ifndef EVENTS_H
#define EVENTS_H
#include <unordered_map>
#include <sys/epoll.h>
#include <shared.hpp>
#include <eink.hpp>
#include <term.hpp>
#include <pty.hpp>
#include <input.hpp>
#include <string>

const int MAX_EVENTS = 128;

class EventLoop {

    private:
        EventLoop() = default;
        ~EventLoop() = default;
    public:

        EventLoop(const EventLoop&) = delete;
        EventLoop& operator=(const EventLoop&) = delete;

        static EventLoop& instance() {
            static EventLoop eventloop; 
            return eventloop;
        }

        bool start();
        bool stop();
        bool loop();

        bool registerCallback(int fd, Module* mod);
        bool handleTransfer(); 

        int epoll_fd;
        struct epoll_event events[MAX_EVENTS];
        std::unordered_map<int, Module*>cb_map;
        int epoll_timeout;

        EInk eink;
        Terminal terminal;    
        PTY pty;
        Input input;
    
        std::string shell;

        std::string buf_key2pty;
        std::string buf_pty2term;
};


#endif
