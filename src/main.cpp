#include <iostream>
#include <fbink.h>
#include <vterm.h>

int main() {
    std::cout << "Hello World!" << std::endl;
    std::cout << std::string(fbink_version()) << std::endl;
    std::cout << vterm_new(20, 20) << std::endl;

    return 0;
}
