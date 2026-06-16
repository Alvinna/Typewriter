#include <string>
#include <eink.hpp>
#include <term.hpp>
#include <pty.hpp>

int main() {
    
    EInk eink;
    Terminal terminal;    
    PTY pty;

    std::string s = std::string("I\'m in ;)");
    std::string cmd = std::string("/bin/sh");
    std::string arg = std::string("");
    std::string c = std::string("pwd");

    eink.open();
    terminal.open(eink.getRows(), eink.getCols());
    pty.open(cmd, arg);

    eink.printState(); 
    terminal.write(s);
    pty.setSize(eink.getRows(), eink.getCols());
    pty.write(c);
    
    terminal.close();
    eink.close();

    return 0;
}


