#include <iostream>
#include <term.hpp>

bool Terminal::open(int rows, int cols) {


    // Allocate new virtual terminal
    term = vterm_new(rows, cols);
    
    // Use UTF8
    vterm_set_utf8(term, 1);

    screen = vterm_obtain_screen(term);
    
    // Register callback
    cb = (VTermScreenCallbacks){
        .damage = Terminal::damage,
        .moverect = Terminal::moverect,
        .movecursor = Terminal::movecursor,
        .settermprop = Terminal::settermprop,
        .bell = Terminal::bell,
        .resize = Terminal::resize,
        .sb_pushline = Terminal::sb_pushline,
        .sb_popline = Terminal::sb_popline,
        .sb_clear = Terminal::sb_clear,
        .sb_pushline4 = NULL
    };
    vterm_screen_set_callbacks(screen, &cb, this);
    
    vterm_screen_enable_altscreen(screen, 1);
    
    // Reset screen
    vterm_screen_reset(screen, 1);
    
    return true;
}

bool Terminal::close() {
    vterm_free(term);
    return true;
}


bool Terminal::write(const std::string& text) {
    vterm_input_write(term, text.c_str(), text.length());
    return true;
}

// Implement screen update behavior here!
int Terminal::damage(VTermRect rect, void *user) {
    // Get pointer to terminal object
    Terminal* self = static_cast<Terminal*>(user);
    std::cout << "damage" << std::endl;

    return 1;
}
int Terminal::moverect(VTermRect dest, VTermRect src, void *user) {
    // Get pointer to terminal object
    Terminal* self = static_cast<Terminal*>(user);
    std::cout << "moverect" << std::endl;
    return 1;
}
int Terminal::movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user) {
    // Get pointer to terminal object
    Terminal* self = static_cast<Terminal*>(user);
    std::cout << "movecursor" << std::endl;

    return 1;
}
int Terminal::settermprop(VTermProp prop, VTermValue *val, void *user) {
    // Get pointer to terminal object
    Terminal* self = static_cast<Terminal*>(user);
    std::cout << "settermprop" << std::endl;

    return 1;
}
int Terminal::bell(void *user) {
    // Get pointer to terminal object
    Terminal* self = static_cast<Terminal*>(user);
    std::cout << "bell" << std::endl;

    return 0;
}
int Terminal::resize(int rows, int cols, void *user) {
    // Get pointer to terminal object
    Terminal* self = static_cast<Terminal*>(user);
    std::cout << "resize" << std::endl;

    return 1;
}
int Terminal::sb_pushline(int cols, const VTermScreenCell *cells, void *user) {
    // Get pointer to terminal object
    Terminal* self = static_cast<Terminal*>(user);
    std::cout << "sb_pushline" << std::endl;

    return 1;
}
int Terminal::sb_popline(int cols, VTermScreenCell *cells, void *user) {
    // Get pointer to terminal object
    Terminal* self = static_cast<Terminal*>(user);
    std::cout << "sb_popline" << std::endl;

    return 1;
}
int Terminal::sb_clear(void *user) {
    // Get pointer to terminal object
    Terminal* self = static_cast<Terminal*>(user);
    std::cout << "sb_clear" << std::endl;

    return 1;
}
