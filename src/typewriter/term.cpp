#include <iostream>
#include <vterm.h>
#include <term.hpp>
#include <events.hpp>
#include <eink.hpp>
#include <sys/time.h>
#include <sys/timerfd.h>

bool Terminal::open() {


    if (!eink.open())
        return false;

    // Hard coded for now
    const std::string fr("IBMPlexMono-Regular.ttf");
    const std::string fb("IBMPlexMono-Bold.ttf");
    const std::string fi("IBMPlexMono-Italic.ttf");
    const std::string fbi("IBMPlexMono-BoldItalic.ttf");

    eink.loadFonts(fr, fb, fi, fbi);
    eink.calcCharSize();
    eink.calculateWindowSize();

    
    // Allocate new virtual terminal
    term = vterm_new(eink.rows, eink.cols);
     
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
    
    // Create damage timer
    timer_fd_dmg = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        if (timer_fd_dmg == -1) {
            perror("Error creating timer");
            return false;
        }
   
    ts_dmg.it_value.tv_nsec = DMG_INTERVAL;
    timerfd_settime(timer_fd_dmg, 0, &ts_off, NULL);


    // Create update timer
    timer_fd_upd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        if (timer_fd_upd == -1) {
            perror("Error creating timer");
            return false;
        }
   
    ts_upd.it_value.tv_nsec = UPD_INTERVAL;
    timerfd_settime(timer_fd_upd, 0, &ts_off, NULL);


    // Register timer event
    EventLoop::instance().registerCallback(timer_fd_dmg, EPOLLIN, this);
    EventLoop::instance().registerCallback(timer_fd_upd, EPOLLIN, this);

    should_update = false;
    should_update_cursor = false;

    cursor_type = EINK_CURSOR_HORIZONTAL;
    cursor_visible = true;

    return true;
    
}



bool Terminal::updateScreen(int row, int col, int height, int width) {
    
    char lbuf[TERMINAL_MAX_LINE_WIDTH];
    int last_row, last_col;

    last_row = row;
    last_col = col;

    // std::cout << "Update screen" << std::endl;

    for (int r = row; r < row + height; r++) {

        VTermRect extent;
        extent.start_row = r;
        extent.start_col = col;
        extent.end_row = r + 1;
        extent.end_col = col + width;

        int l = vterm_screen_get_text(screen, lbuf, width, extent);

        if (l == 0) {
            eink.clearRect(r, col, 1, width);
        }
        else {


            if (l < width) {
                eink.clearRect(r, l, 1, (width - l));
            }

            for (int c = col; c < col + l;) {
                VTermPos pos_start;
                VTermScreenCell cell;
                pos_start.col = c;
                pos_start.row = r;
                
                extent.start_row = r;
                extent.start_col = c;
                extent.end_row = r + 1;
                extent.end_col = col + l;

                vterm_screen_get_attrs_extent(screen, &extent, pos_start,  VTERM_ALL_ATTRS_MASK);
                
                // std::cout << extent.start_row << ", " << extent.start_col << ", " <<
                //     extent.end_row << ", " << extent.end_col << std::endl;
                
                int w = extent.end_col - extent.start_col + 1;
                
                vterm_screen_get_cell(screen, pos_start, &cell);
                
                if (cell.attrs.bold && cell.attrs.italic) {
                    eink.ot_config.style = FNT_BOLD_ITALIC;
                }
                else if (cell.attrs.bold && !cell.attrs.italic) {
                    eink.ot_config.style = FNT_ITALIC;
                }
                else if (!cell.attrs.bold && cell.attrs.italic) {
                    eink.ot_config.style = FNT_BOLD;
                }
                else {
                    eink.ot_config.style = FNT_REGULAR;
                }

                if (cell.attrs.reverse) {
                    eink.config.is_inverted = true;
                }
                else {
                    eink.config.is_inverted = false;
                }
                
                eink.printText(r, c, std::string(&lbuf[c - col], w));
                c = extent.end_col + 1;
                
                eink.config.is_inverted = false;
                eink.ot_config.style = FNT_REGULAR;

            }

        }
        
    }

    return true;
}

bool Terminal::updateCursor(int new_row, int new_col, int old_row, int old_col, bool clearOld) {
    // TODO: deal with changing cursor type
    
    if (cursor_visible) {
        if (clearOld) {
            eink.invertCursor(old_row, old_col, cursor_type);
        }

        eink.invertCursor(new_row, new_col, cursor_type);
    } 

    return true;
}

bool Terminal::close() {
    vterm_free(term);

    eink.freeFonts();
    return eink.close();
}


bool Terminal::write(const std::string& text) {
    vterm_input_write(term, text.c_str(), text.length());
    return true;
}

// Implement screen update behavior here!
int Terminal::damage(VTermRect rect, void *user) {
    // Get pointer to terminal object
    Terminal* self = static_cast<Terminal*>(user);
    //std::cout << "damage(" << rect.start_row << ", " << rect.start_col << 
    //    ", " << rect.end_row << ", " << rect.end_col << ")"<< std::endl;
   
    if (self->upd_timer_on == false) {
        self->upd_timer_on = true;
        timerfd_settime(self->timer_fd_upd, 0, &self->ts_upd, 0);
    }

    self->should_update = true;
    self->update_start_row = std::min(self->update_start_row, rect.start_row);
    self->update_start_col = std::min(self->update_start_col, rect.start_col);
    self->update_end_row = std::max(self->update_end_row, rect.end_row);
    self->update_end_col = std::max(self->update_end_col, rect.end_col);

    // Reset timer
    timerfd_settime(self->timer_fd_dmg, 0, &(self->ts_dmg), 0);

    return 1;
}
int Terminal::moverect(VTermRect dest, VTermRect src, void *user) {
    // Get pointer to terminal object
    Terminal* self = static_cast<Terminal*>(user);
    // std::cout << "moverect([" << dest.start_row << ", " << dest.start_col << 
    //     ", " << dest.end_row << ", " << dest.end_col << 
    //     src.start_row << ", " << src.start_col << 
    //     ", " << src.end_row << ", " << src.end_col << "])"<< std::endl;

    damage(dest, user);
    return 1;
}
int Terminal::movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user) {
    // Get pointer to terminal object
    Terminal* self = static_cast<Terminal*>(user);
    //std::cout << "movecursor([" << pos.row << ", " << pos.col << "], [" << 
      //  oldpos.row << ", " << oldpos.col << "])" << std::endl;
    

    if (self->should_update_cursor == false) {
        self->cursor_old_row = oldpos.row;
        self->cursor_old_col = oldpos.col;
    }

    self->cursor_new_row = pos.row;
    self->cursor_new_col = pos.col;

    self->should_update_cursor = true;

    timerfd_settime(self->timer_fd_dmg, 0, &(self->ts_dmg), 0);

    return 1;
}
int Terminal::settermprop(VTermProp prop, VTermValue *val, void *user) {
    // Get pointer to terminal object
    Terminal* self = static_cast<Terminal*>(user);
    // std::cout << "settermprop" << std::endl;
    VTermRect rect;
    VTermPos pos;


    switch (prop) {
        case VTERM_PROP_CURSORVISIBLE:
            self->cursor_visible = val->boolean;
            break;
        case VTERM_PROP_CURSORBLINK:
            break;
        case VTERM_PROP_CURSORSHAPE:
            if (val->number == VTERM_PROP_CURSORSHAPE_BLOCK)
                self->cursor_type = EINK_CURSOR_BLOCK;
            else if (val->number == VTERM_PROP_CURSORSHAPE_BAR_LEFT)
                self->cursor_type = EINK_CURSOR_VERTICAL;
            else
                self->cursor_type = EINK_CURSOR_HORIZONTAL;
            
            pos.col = self->cursor_new_col;
            pos.row = self->cursor_new_row;
            //movecursor(pos, pos, self->cursor_visible?1:0, user);

            break;
        case VTERM_PROP_ALTSCREEN:
            rect.start_row = 0;
            rect.start_col = 0;
            rect.end_row = self->eink.rows;
            rect.end_col = self->eink.cols;
            damage(rect, user);
            break;
        default:
            break;
        }

    return 1;
}
int Terminal::bell(void *user) {
    // Get pointer to terminal object
    Terminal* self = static_cast<Terminal*>(user);
    // std::cout << "bell" << std::endl;

    return 0;
}
int Terminal::resize(int rows, int cols, void *user) {
    // Get pointer to terminal object
    Terminal* self = static_cast<Terminal*>(user);
    std::cout << "resize" << std::endl;
    std::cout << "resize(" << rows << ", " << cols << ")" << std::endl;

    return 1;
}
int Terminal::sb_pushline(int cols, const VTermScreenCell *cells, void *user) {
    // Get pointer to terminal object
    Terminal* self = static_cast<Terminal*>(user);
    // std::cout << "sb_pushline(" << cols << ")" << std::endl;

    return 1;
}
int Terminal::sb_popline(int cols, VTermScreenCell *cells, void *user) {
    // Get pointer to terminal object
    Terminal* self = static_cast<Terminal*>(user);
    // std::cout << "sb_popline(" << cols << ")" << std::endl;

    return 1;
}
int Terminal::sb_clear(void *user) {
    // Get pointer to terminal object
    Terminal* self = static_cast<Terminal*>(user);
    std::cout << "sb_clear" << std::endl;

    return 1;
}



bool Terminal::handleEvent(int fd, struct epoll_event* event) {

    if (fd == timer_fd_dmg) {

        // std::cout << "DMG timer" << std::endl;
        if (should_update || should_update_cursor) {

            eink.config.no_refresh = true;

            eink.ot_config.padding = FULL_PADDING;
            
            if (should_update) {
                updateScreen(update_start_row, update_start_col, 
                        update_end_row - update_start_row, update_end_col - update_start_col);
            }

            if (should_update_cursor) {
                // If old cursor is within update region, or cursor doesn move
                // (but might change type), no need to redraw
                // old one since it has been 
                if ((should_update && cursor_old_row >= update_start_row && cursor_old_row < update_end_row &&
                            cursor_old_col >= update_start_col && cursor_old_col < update_end_col) || 
                        (cursor_old_row == cursor_new_row && cursor_old_col == cursor_new_col))
                    updateCursor(cursor_new_row, cursor_new_col, cursor_old_row, cursor_old_col, false); 

                else
                    updateCursor(cursor_new_row, cursor_new_col, cursor_old_row, cursor_old_col, true); 


            }


            eink.config.no_refresh = false;
            eink.refreshRect(0, 0, eink.rows, eink.cols);

            should_update_cursor = false;
            should_update = false;
            timerfd_settime(timer_fd_dmg, 0, &ts_off, 0);
            timerfd_settime(timer_fd_upd, 0, &ts_upd, 0);
            upd_timer_on = true;
        }

    }
    else if (fd == timer_fd_upd) {
        // std::cout << "UPD timer" << std::endl;

        if (should_update || should_update_cursor) {


            eink.config.no_refresh = true;

            eink.ot_config.padding = FULL_PADDING;
            
            if (should_update) {
                updateScreen(update_start_row, update_start_col, 
                        update_end_row - update_start_row, update_end_col - update_start_col);
            }

            if (should_update_cursor) {
                // If old cursor is within update region, or cursor doesn move
                // (but might change type), no need to redraw
                // old one since it has been 
                if ((should_update && cursor_old_row >= update_start_row && cursor_old_row < update_end_row &&
                            cursor_old_col >= update_start_col && cursor_old_col < update_end_col) || 
                        (cursor_old_row == cursor_new_row && cursor_old_col == cursor_new_col))
                    updateCursor(cursor_new_row, cursor_new_col, cursor_old_row, cursor_old_col, false); 

                else
                    updateCursor(cursor_new_row, cursor_new_col, cursor_old_row, cursor_old_col, true); 

            }


            eink.config.no_refresh = false;
            eink.refreshRect(0, 0, eink.rows, eink.cols);

            should_update_cursor = false;
            should_update = false;
            timerfd_settime(timer_fd_dmg, 0, &ts_off, 0);
            timerfd_settime(timer_fd_upd, 0, &ts_upd, 0);
            upd_timer_on = true;

        }
        else {
            timerfd_settime(timer_fd_upd, 0, &ts_off, 0);
            upd_timer_on = false;
        }

    }

    return true;

}
