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
    
    config.is_quiet = true;
    config.is_verbose = false;
    config.saturation_boost = 50;

    fbink_update_verbosity(&config);
    

    return true;
}

bool EInk::close() {

    fbink_close(fd);

    return true;
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

bool EInk::calcCharSize() {
    
    bool c = ot_config.compute_only;
    ot_config.compute_only = true;
    ot_fit = {};

    // Use a full block character to calculate monospace font's size ;)
    int ret = fbink_print_ot(fd, "████████", &ot_config, &config, &ot_fit);
    char_width = ot_fit.bbox.width >> 3;
    char_height = ot_fit.bbox.height;

    ot_config.compute_only = c;
    return true;

}


bool EInk::calculateWindowSize() {

    int s_width = state.view_width;
    int s_height = state.view_height;

    int d_width = s_width - 2 * pad_lr;
    int d_height = s_height - 2 * pad_td;

    int e_width = (int)(d_width / char_width) * char_width;
    int e_height = (int)(d_height / char_height) * char_height;
    
    offset_l = pad_lr + (d_width - e_width) / 2;
    offset_t = pad_td + (d_height - e_height) / 2;

    cols = e_width / char_width;
    rows = e_height / char_height;
    
    return true;
}


bool EInk::printText(int row, int col, const std::string& text) {

    ot_config.margins.top = offset_t + row * char_height;
    ot_config.margins.left = offset_l + col * char_width;
    //std::cout << text.c_str() << std::endl;
    // std::cout << ot_config.margins.top << ", " << ot_config.margins.left << std::endl;
    fbink_print_ot(fd, text.c_str(), &ot_config, &config, NULL);
    
    return true;

}


bool EInk::refreshRect(int row, int col,
                 int height, int width) {
    
    int y1 = row * char_height + offset_t;
    int x1 = col * char_width + offset_l;
    fbink_refresh(fd, y1, x1, width * char_width, height * char_height, 
            &config);
    return true;
}

bool EInk::clearRect(int row, int col,
                 int height, int width) {
    
    int y1 = row * char_height + offset_t;
    int x1 = col * char_width + offset_l;
    FBInkRect rect;
    rect.top = y1;
    rect.left = x1;
    rect.width = width * char_width;
    rect.height = height * char_height;
    
    fbink_cls(fd, &config, &rect, false);

    return true;
}



bool EInk::loadFonts(const std::string& font_regular, const std::string& font_bold,
        const std::string& font_italic, const std::string& font_bold_italic) {

    int ret = 0;

    ret |= fbink_add_ot_font_v2(font_regular.c_str(), FNT_REGULAR, &ot_config);
    ret |= fbink_add_ot_font_v2(font_bold.c_str(), FNT_BOLD, &ot_config);
    ret |= fbink_add_ot_font_v2(font_italic.c_str(), FNT_ITALIC, &ot_config);
    ret |= fbink_add_ot_font_v2(font_bold_italic.c_str(), FNT_BOLD_ITALIC, &ot_config);
    
    ot_config.is_formatted = false;
    ot_config.size_pt = 14;
    ot_config.padding = 0;

    return !ret;

}

bool EInk::freeFonts() {

    fbink_free_ot_fonts_v2(&ot_config);
    return true;
}



bool EInk::invertCursor(int row, int col, EInkCursorType type) {
    const int cursor_width = 2;

    int y1 = row * char_height + offset_t;
    int x1 = col * char_width + offset_l;
    FBInkRect rect;

    switch (type) {
        case EINK_CURSOR_BLOCK:
            rect.top = y1;
            rect.left = x1;
            rect.width = char_width * 1;
            rect.height = char_height * 1;
            break;
        case EINK_CURSOR_VERTICAL:
            rect.top = y1;
            rect.left = x1;
            rect.width = 2;
            rect.height = char_height * 1;
            break;
        case EINK_CURSOR_HORIZONTAL:
            rect.top = y1 + char_height - 4;
            rect.left = x1;
            rect.width = char_width;
            rect.height = 4;
            break;
    }

    fbink_invert_rect(fd, &rect, false);
    return true;
}
