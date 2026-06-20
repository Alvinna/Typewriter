#ifndef TERM_H
#define TERM_H

#include <string>
#include <vterm.h>
#include <shared.hpp>
#include <eink.hpp>
#include <sys/time.h>

constexpr int DMG_INTERVAL = 10e6; // 10 ms
constexpr int UPD_INTERVAL = 500e6; // 0.5s
constexpr int TERMINAL_MAX_LINE_WIDTH = 1024;

class Terminal : public Module{

    public:
        VTerm *term;
        VTermScreen *screen;
        VTermScreenCallbacks cb;
        EInk eink;
        int timer_fd_dmg;
        int timer_fd_upd;
        itimerspec ts_off = {};
        itimerspec ts_dmg = {};
        itimerspec ts_upd = {};
        bool upd_timer_on = false;

        bool should_update = false;
        int update_start_row = 0;
        int update_start_col = 0;
        int update_end_row = 0;
        int update_end_col = 0;

        bool open();
        bool close();

        bool write(const std::string &text);

        bool updateScreen(int row, int col, int height, int width);

        bool handleEvent(int fd, struct epoll_event* event);

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
