#ifndef SHARED_H
#define SHARED_H
#include <sys/epoll.h>

class Module {

    public:
        virtual bool handleEvent(int fd, struct epoll_event* event) = 0;

};



#endif
