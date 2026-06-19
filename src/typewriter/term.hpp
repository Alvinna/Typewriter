#ifndef TERM_H
#define TERM_H

#include <string>
#include <vterm.h>
#include <eink.hpp>

class Terminal {

    public:
        VTerm *term;
        VTermScreen *screen;
        VTermScreenCallbacks cb;
        EInk eink;

        bool open();
        bool close();

        bool write(const std::string &text);

        static int damage(VTermRect rect, void *user);
        static int moverect(VTermRect dest, VTermRect src, void *user);
        static int movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user);
        static int settermprop(VTermProp prop, VTermValue *val, void *user);
        static int bell(void *user);
        static int resize(int rows, int cols, void *user);
        static int sb_pushline(int cols, const VTermScreenCell *cells, void *user);
        static int sb_popline(int cols, VTermScreenCell *cells, void *user);
        static int sb_clear(void *user);
};


#endif
