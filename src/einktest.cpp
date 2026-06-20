#include <eink.hpp>
#include <iostream>
#include <fbink.h>
#include <random>
#include <algorithm>
#include <chrono>
#include <thread>

std::string generate_random_string(size_t length) {
    const std::string characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()_+-=[]{};:";
    std::random_device random_device;
    std::mt19937 generator(random_device());

    std::string random_string(characters);
    std::shuffle(random_string.begin(), random_string.end(), generator);

    return random_string.substr(0, length);
}


int main(int argc, char** argv) {
    std::string s;

    for (int i = 0; i < std::stoi(std::string(argv[1])); i++) {
        s = generate_random_string(i % 50);
        std::cout << s << std::endl;
        std::this_thread::sleep_for (std::chrono::milliseconds(2));
    }

    return 0;
}
