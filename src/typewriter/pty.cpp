#include <iostream>
#include <unistd.h>
#include <pty.hpp>
#include <sys/wait.h>
#include <cstring>
#include <events.hpp>

extern char **environ;

bool PTY::open(const std::string& shell) {

    pid = forkpty(&master, 0, 0, 0);
    
    if (pid < 0) {
        std::cout << "Error forking PTY" << std::endl;
        return false;
    } 
    else if (pid == 0) {
        // Child
        char * t = (char*)malloc(shell.length() + 1);
        memcpy(t, shell.c_str(), shell.length()); 
        t[shell.length()] = '\0';

        char * const args[] = {t, 0};
        execve(shell.c_str(), args, environ);
    } 
    else {
        // Parent
        tcgetattr(master, &tios);
        // tios.c_lflag |= ECHO;
        // tios.c_lflag ^= ECHONL;
        tios.c_lflag &= ~ECHO; 
        tios.c_lflag &= ~ICANON;
        tcsetattr(master, TCSAFLUSH, &tios);
        
        int flags = fcntl(master, F_GETFL, 0);
        fcntl(master, F_SETFL, flags | O_NONBLOCK);
        
        EventLoop::instance().registerCallback(master, this);

    }

    return true;
}

bool PTY::close() {
    wait(NULL);
    return true;
}

bool PTY::setSize(int rows, int cols) {
    struct winsize ws;
    ws.ws_row = (short)rows;
    ws.ws_col = (short)cols;

    ioctl(master, TIOCSWINSZ, &ws);

    return true;
}


void PTY::write(std::string& text) {
    // System call
    ::write(master, text.c_str(), text.length());
}


bool PTY::handleEvent(int fd) {

    int n = ::read(master, buf, sizeof(buf));
    
    if (n > 0) {
        EventLoop::instance().buf_pty2term.insert(
                EventLoop::instance().buf_pty2term.length(), 
                buf, n);
    }

    return true;
}
