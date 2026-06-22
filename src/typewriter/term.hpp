#ifndef TERM_H
#define TERM_H

#include <string>
#include <vterm.h>
#include <shared.hpp>
#include <eink.hpp>
#include <sys/time.h>

constexpr int DMG_INTERVAL = 10e6; // 
constexpr int UPD_INTERVAL = 500e6; // 0.5s
constexpr int TERMINAL_MAX_LINE_WIDTH = 1024;

class Terminal : public Module{

    public:
        VTerm *term;
        VTermScreen *screen;
        VTermScreenCallbacks cb;
        EInk eink;
        
        bool cursor_visible;
        EInkCursorType cursor_type, cursor_type_old;
        int timer_fd_dmg;
        int timer_fd_upd;
        itimerspec ts_off = {};
        itimerspec ts_dmg = {};
        itimerspec ts_upd = {};
        bool upd_timer_on = false;

        bool should_update = false;
        int update_start_row = 1337;
        int update_start_col = 1337;
        int update_end_row = 0;
        int update_end_col = 0;

        bool should_update_cursor = false;
        int cursor_old_row = 0;
        int cursor_old_col = 0;
        int cursor_new_row = 0;
        int cursor_new_col = 0;
        bool cursor_update_type = false;
        
        int input_count = 0;
        bool writer_mode = false;
        bool writer_first_time = true;

        bool open();
        bool close();

        bool write(const std::string &text);

        bool updateScreen(int row, int col, int height, int width);
        bool updateCursor(int new_row, int new_col, int old_row, int old_col, bool clearOld);

        void update_fg_color(VTermColor * c);
        void update_bg_color(VTermColor * c);


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
