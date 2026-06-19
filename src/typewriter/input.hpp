#ifndef INPUT_H
#define INPUT_H
#include <vector>
#include <sys/epoll.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <string>
#include <fcntl.h>
#include <deque>
#include <keymap.hpp>
#include <shared.hpp>

const int inputs_max_events = 128;

struct EventDevice {
    int fd;
    struct libevdev *dev;
    std::string name;
    std::string path;
};

struct InputDevices {
    EventDevice power_key;
    EventDevice gpio_key;
    EventDevice touchscreen;
    std::vector<EventDevice> keyboards;
};

class Input : public Module{

    public:
        std::deque<char> keys;

        bool open();
        bool close();

        bool handleEvent(int fd, struct epoll_event* event);

    private:
        InputDevices devices;
        KeycodeTranslation key_translator;

        bool populateDevices();

};


#endif
