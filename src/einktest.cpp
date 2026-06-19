#include <eink.hpp>
#include <iostream>
#include <fbink.h>
#include <random>
#include <algorithm>

std::string generate_random_string(size_t length) {
    const std::string characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()_+-=[]{};:";
    std::random_device random_device;
    std::mt19937 generator(random_device());

    std::string random_string(characters);
    std::shuffle(random_string.begin(), random_string.end(), generator);

    return random_string.substr(0, length);
}


int main() {
    EInk eink;

    eink.open();
    eink.printState();

    if (fbink_add_ot_font_v2("IBMPlexMono-Regular.ttf", FNT_REGULAR, &eink.ot_config) != 0) {
        std::cout << "Error loading font" << std::endl;
    }
    if (fbink_add_ot_font_v2("IBMPlexMono-Bold.ttf", FNT_BOLD, &eink.ot_config) != 0) {

        std::cout << "Error loading font" << std::endl;
    }
    if (fbink_add_ot_font_v2("IBMPlexMono-Italic.ttf", FNT_ITALIC, &eink.ot_config) != 0) {

        std::cout << "Error loading font" << std::endl;
    }
    if (fbink_add_ot_font_v2("IBMPlexMono-BoldItalic.ttf", FNT_BOLD_ITALIC, &eink.ot_config) != 0) {

        std::cout << "Error loading font" << std::endl;
    }
    
    eink.ot_config.is_formatted = false;
    eink.ot_config.size_pt = 13;
    eink.ot_config.padding = 10;

    eink.calcCharSize();
    eink.calculateWindowSize();

    std::cout << "rows, cols = " << eink.rows << ", " << eink.cols << std::endl;
    std::cout << "offset t, l = " << eink.offset_t << ", " << eink.offset_l << std::endl;
    
    std::string s;

    eink.config.no_refresh = true;

    for (int i = 0; i < eink.rows; i++) {
        if (i % 4 == 0)
            eink.ot_config.style = FNT_REGULAR;
        else if (i % 4 == 1)
            eink.ot_config.style = FNT_ITALIC;
        else if (i % 4 == 2)
            eink.ot_config.style = FNT_BOLD;
        else if (i % 4 == 3)
            eink.ot_config.style = FNT_BOLD_ITALIC;

        s = generate_random_string(eink.cols - i);
        eink.printText(i, i, s);
    }

    eink.config.no_refresh = false;
    eink.refreshRect(0, 0, eink.rows, eink.cols);

    fbink_free_ot_fonts_v2(&eink.ot_config);


    eink.close();


    return 0;
}
