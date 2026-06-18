#ifndef PTY_H
#define PTY_H
#include <string>
#include <pty.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/epoll.h>

const int pty_max_events = 128;
const int pty_max_read_size = 4096;

class PTY {

    public:
        pid_t pid;
        int master;
        struct termios tios;
        
        bool open(const std::string& shell);
        bool close();
        bool setSize(int rows, int cols);
        void write(std::string& text);
        int read(char* buf, int length);

    private:
        int epoll_fd;
        struct epoll_event events[pty_max_events];

};


#endif
