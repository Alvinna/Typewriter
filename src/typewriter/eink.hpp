#ifndef EINK_H
#define EINK_H

#include <fbink.h>

class EInk {

    public:
        int fd;
        FBInkConfig config = {0};
        FBInkState state = {0};

        bool open();
        bool close();
        bool printState();


};


#endif
