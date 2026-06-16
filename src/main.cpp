#include <iostream>
#include <fbink.h>
#include <vterm.h>
#include <libevdev-1.0/libevdev/libevdev.h>


int main() {
    std::cout << "Hello World!" << std::endl;
    std::cout << std::string(fbink_version()) << std::endl;
    std::cout << vterm_new(20, 20) << std::endl;
    std::cout << libevdev_event_code_from_code_name("KEY_A") << std::endl;

    return 0;
}
