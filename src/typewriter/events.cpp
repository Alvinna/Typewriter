#include <events.hpp>
#include <cstdio>
#include <sys/epoll.h>
#include <unistd.h>
#include <shared.hpp>
#include <iostream>


bool EventLoop::start() {
    

    shell = std::string("/bin/sh");
    struct epoll_event event;
    
    event.events = EPOLLIN | EPOLLET;
    epoll_fd = epoll_create1(0);

    if (epoll_fd == -1) {
        perror("Error creating epoll handle");
        return false;
    }
    epoll_timeout = 10;

    buf_key2pty.clear();
    buf_pty2term.clear();

    terminal.open();
    pty.open(shell);
    pty.setSize(terminal.eink.rows, terminal.eink.cols);
    input.open();
    
    return true;
}

bool EventLoop::stop() {

    if (close(epoll_fd) == -1) {
        perror("Error closing epoll handle");
        return false;
    }


    input.close();
    pty.close();
    terminal.close();

    return true;
}

bool EventLoop::loop() {
    bool should_exit = false;
    
    while(true) {

        int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, epoll_timeout);

        if (n_events < 0) {
            perror("Error polling");
            return false;
        }
        else if (n_events == 0) {
            continue;
        }
        
        for (int i = 0; i < n_events; i++) {
            if (cb_map.find(events[i].data.fd) != cb_map.end()) {
                if (!(cb_map[events[i].data.fd])->handleEvent(events[i].data.fd, &events[i]))
                    should_exit = true;
                    break;
            }
        }

        handleTransfer();
        
        if (should_exit)
            break;
        
    }

    return true;
}

bool EventLoop::registerCallback(int fd, uint32_t event_type, Module* mod) {

    struct epoll_event event;
    event.events = event_type;

    event.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
        perror("Error registering epoll for input");
        return false;
    }

    cb_map[fd] = mod;

    return true;

}

bool EventLoop::handleTransfer() {
    pty.write(buf_key2pty);
    buf_key2pty.clear();
    std::cout << buf_pty2term << std::flush;
    terminal.write(buf_pty2term);
    buf_pty2term.clear();
    
    return true;
}
