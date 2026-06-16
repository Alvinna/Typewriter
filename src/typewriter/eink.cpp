#include <eink.hpp>
#include <iostream>

bool EInk::open() {
    
    fd = fbink_open();

    if (fd == -1) {
        std::cout << "Error opening framebuffer device" << std::endl;
        return false;
    }

    // Initialize framebuffer
    if (fbink_init(fd, &config) != 0) {
        std::cout << "Error initializing framebuffer device" << std::endl;
        return false;
    }

    // Clear screen
    fbink_cls(fd, &config, nullptr, false);

    // Get framebuffer states
    fbink_get_state(&config, &state);

    return true;
}

bool EInk::close() {

    fbink_close(fd);

    return true;
}

int EInk::getCols() {
    return state.max_cols;
}

int EInk::getRows() {
    return state.max_rows;
}

bool EInk::printState() {

    std::cout << "device_name: " << state.device_name << std::endl;
    std::cout << "device_codename: " << state.device_name << std::endl;
    std::cout << "device_platform: " << state.device_platform << std::endl;
    std::cout << "device_id: " << state.device_id << std::endl;
    std::cout << "view_width: " << state.view_width << std::endl;
    std::cout << "view_height: " << state.view_height << std::endl;

    return true;
}
