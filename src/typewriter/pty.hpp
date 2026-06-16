#ifndef PTY_H
#define PTY_H
#include <string>
#include <pty.h>
#include <termios.h>
#include <fcntl.h>

class PTY {

    public:
        pid_t pid;
        int master;
        struct termios tios;
        
        bool open(std::string& cmd, std::string& arg);
        bool close();
        bool setSize(int rows, int cols);
        void write(std::string& text);

};


#endif
