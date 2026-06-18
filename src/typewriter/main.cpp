#include <string>
#include <eink.hpp>
#include <term.hpp>
#include <pty.hpp>
#include <input.hpp>
#include <iostream>

int main() {
    
    EInk eink;
    Terminal terminal;    
    PTY pty;
    Input input;

    std::string s = std::string("I\'m in ;)");
    std::string cmd = std::string("/bin/sh");
    std::string c = std::string("pwd");
    
    char b[65536]; 

    eink.open();
    terminal.open(eink.getRows(), eink.getCols());
    pty.open(cmd);
    input.open();

    eink.printState(); 
    terminal.write(s);
    pty.setSize(eink.getRows(), eink.getCols());
   
    std::string tty_input;
    for (int i = 0; i < 10000; i++) {
        tty_input.clear();

        input.getEvents();
        while (input.keys.size() > 0) {
            tty_input.push_back(input.keys.front());
            input.keys.pop_front();
        }

        if (tty_input.length() > 0) {
            //std::cout << tty_input << std::endl;
            pty.write(tty_input);
        }
        
        int n = pty.read(b, 65536);
        if (n > 0) {
            //terminal.write(std::string(b, n)); 
            std::cout << std::string(b, n) << std::flush;
        }
    }
    std::cout << "times up" << std::endl;
    pty.close();
    terminal.close();
    eink.close();

    return 0;
}


