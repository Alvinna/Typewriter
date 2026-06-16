#include <eink.hpp>

int main() {
    
    EInk eink;

    eink.open();
    eink.printState();
    eink.close();

    return 0;
}


