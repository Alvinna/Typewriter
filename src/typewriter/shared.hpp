#ifndef SHARED_H
#define SHARED_H


class Module {

    public:
        virtual bool handleEvent(int fd) = 0;

};



#endif
