#ifndef PTY_H
#define PTY_H
#include <shared.hpp>
#include <string>
#include <pty.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/epoll.h>

const int PTY_BUFFER_SIZE = 65536;

class PTY : public Module{

    public:
        pid_t pid;
        int master;
        struct termios tios;
        
        bool open(const std::string& shell);
        bool close();
        bool setSize(int rows, int cols);
        void write(std::string& text);

        bool handleEvent(int fd, struct epoll_event* event);

    
        char buf[PTY_BUFFER_SIZE];

};


#endif
