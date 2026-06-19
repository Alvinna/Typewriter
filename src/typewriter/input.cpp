#include <iostream>
#include <filesystem>
#include <regex>
#include <vector>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <input.hpp>
#include <events.hpp>


bool Input::open() {
    
    populateDevices();

    for (auto d : devices.keyboards) {
        EventLoop::instance().registerCallback(d.fd, EPOLLIN | EPOLLET, this);        
    }
    
    return true;

}

bool Input::close() {

    for (auto d : devices.keyboards) {
        libevdev_free(d.dev);
        ::close(d.fd);
    }

    devices.power_key.fd = -1;
    devices.gpio_key.fd = -1;
    devices.touchscreen.fd = -1;
    devices.keyboards.clear();
    
    return true;
}

bool Input::handleEvent(int fd, struct epoll_event* event) {

    for (auto d : devices.keyboards) {
        if (d.fd == fd) {

            struct input_event ev;
            while (libevdev_next_event(d.dev, LIBEVDEV_READ_FLAG_NORMAL, &ev) == 0) {
                if (ev.type == EV_KEY) {
                    int s = keys.size(); 
                    if (ev.value == 1 || ev.value == 2) {
                        key_translator.press(ev.code, keys);
                    }
                    else {
                        key_translator.release(ev.code, keys);
                    }
                    
                    if (keys.size() > s && key_translator.is_ctrl())
                            keys.front() &= 31;
                    
                }
            }
            break;
        }
    }

    while (keys.size() > 0) {
        EventLoop::instance().buf_key2pty.push_back(keys.front());
        keys.pop_front();
    }

    return true;
}


bool Input::populateDevices() {
    
    const auto input_name_regex = std::regex("event[0-9]+");
    std::vector<std::string> input_devices;
    
    devices.power_key.fd = -1;
    devices.gpio_key.fd = -1;
    devices.touchscreen.fd = -1;
    devices.keyboards.clear();

    // Enumerate input devices
    std::string path = "/dev/input/";
    for (const auto &entry : std::filesystem::directory_iterator(path)) {
        std::string p = entry.path();
        std::string filename = p.substr(p.find_last_of("/\\") + 1);
        
        if (std::regex_match(filename, input_name_regex)) {
            input_devices.push_back(p);
        }

    }

    for (const auto p : input_devices) {

        int fd; 
        struct libevdev *dev = NULL;

        fd = ::open(p.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            std::perror("Failed to open device");
            return false;
        }
        
        if (libevdev_new_from_fd(fd, &dev) < 0) {
            std::perror("Failed to initialize libevdev");
            libevdev_free(dev);
            return false;
        }

        std::string name(libevdev_get_name(dev));
        
        if (libevdev_has_event_type(dev, EV_KEY)) {
            
            if ((name.find("pwrkey") != std::string::npos) || 
                    (name.find("gpio-keys") != std::string::npos) ||
                    (name.find("Touchscreen") != std::string::npos)) {
                // Don't listen for now 
                libevdev_free(dev);
                ::close(fd);
            }
            else {
                devices.keyboards.push_back((EventDevice){.fd = fd, .dev = dev,
                        .name = name, .path = p});
            }
        }
        else {
            // Not a key device
            libevdev_free(dev);
            ::close(fd);
        }
    }
    return true;
}
