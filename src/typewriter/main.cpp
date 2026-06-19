#include <string>
#include <eink.hpp>
#include <term.hpp>
#include <pty.hpp>
#include <input.hpp>
#include <iostream>
#include <events.hpp>

int main() {
    

    EventLoop::instance().start();
    EventLoop::instance().loop();
    EventLoop::instance().stop();

    return 0;
}


