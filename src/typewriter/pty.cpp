#include <iostream>
#include <unistd.h>
#include <pty.hpp>
#include <sys/wait.h>

extern char **environ;

bool PTY::open(std::string& cmd, std::string& arg) {
    
    pid = forkpty(&master, 0, 0, 0);
        
    if (pid < 0) {
        std::cout << "Error forking PTY" << std::endl;
        return false;
    } 
    else if (pid == 0) {
        // Child
        execle(cmd.c_str(), arg.c_str(), environ);
    } 
    else {
        // Parent
        tcgetattr(master, &tios);
        tcsetattr(master, TCSAFLUSH, &tios);
        
        int flags = fcntl(master, F_GETFL, 0);
        fcntl(master, F_SETFL, flags | O_NONBLOCK);
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
