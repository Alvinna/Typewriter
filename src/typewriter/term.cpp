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
    
    const std::string fr2("NotoSansMono-Regular.ttf");
    const std::string fb2("NotoSansMono-Bold.ttf");
    const std::string fi2("NotoSansMono-Regular.ttf");
    const std::string fbi2("NotoSansMono-Regular.ttf");

    eink.loadFonts(fr, fb, fi, fbi);
    // eink.loadFonts(fr2, fb2, fi2, fbi2);
    eink.calcCharSize();
    eink.calculateWindowSize();

    
    // Allocate new virtual terminal
    term = vterm_new(eink.rows, eink.cols);
     
    // Use UTF8
    vterm_set_utf8(term, 0);
     
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
    
    int c_end;
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
           
            
            if (writer_mode && r == cursor_new_row)
                c_end = std::min(col + l, cursor_new_col);
            else
                c_end = col + l;
            

            for (int c = col; c < c_end;) {
                VTermPos pos_start;
                VTermScreenCell cell;
                pos_start.col = c;
                pos_start.row = r;
                
                extent.start_row = r;
                extent.start_col = c;
                extent.end_row = r + 1;
                extent.end_col = c_end;

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
                
                
                update_bg_color(&cell.bg);
                update_fg_color(&cell.fg);

                if (cell.attrs.reverse) {
                    eink.config.is_inverted = true;
                }
                else
                    eink.config.is_inverted = false;
               
                
                if (r == cursor_new_row && writer_mode) { 
                    // eink.printText(r, c, std::string(&lbuf[c - col], std::min(w, c_end - c)));
                    eink.printText(r, c, std::string(&lbuf[c - col], std::min(extent.end_col + 1, cursor_new_col) - col));
                }
                else {
                    eink.printText(r, c, std::string(&lbuf[c - col], w));

                }

                c = extent.end_col + 1;
                
                eink.ot_config.style = FNT_REGULAR;
                
                // if (writer_mode && r == cursor_new_row && c >= c_end)
                //     break;

            }

            eink.config.is_inverted = false;
            fbink_set_fg_pen_rgba(0x00, 0x00, 0x00, 1, false, true);
            fbink_set_bg_pen_rgba(0xFF, 0xFF, 0xFF, 1, false, true);
            
            if (l < width) {
                if (r == cursor_new_row) {
                    if (writer_mode) {
                        eink.clearRect(r, cursor_new_col, 1, (width + col - cursor_new_col));
                        writer_first_time = false;
                    }
                    else if (!writer_mode) {
                        eink.clearRect(r, extent.end_col + 1, 1, (width - l));
                    }
                }
                else {
                    eink.clearRect(r, extent.end_col + 1, 1, (width - l));
                }
            }



        }
        
    }


    return true;
}

bool Terminal::updateCursor(int new_row, int new_col, int old_row, int old_col, bool clearOld) {
    
    if (cursor_visible) {


        if (clearOld || (writer_first_time && writer_mode)) {
            eink.invertCursor(old_row, old_col, cursor_type_old);
        }
        
        if (!writer_mode)
            eink.invertCursor(new_row, new_col, cursor_type);

        cursor_type_old = cursor_type;
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
    // std::cout << "damage(" << rect.start_row << ", " << rect.start_col << 
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
            self->cursor_type_old = self->cursor_type;
            self->cursor_update_type = true;

            if (val->number == VTERM_PROP_CURSORSHAPE_BLOCK) {
                self->cursor_type = EINK_CURSOR_BLOCK;
            }
            else if (val->number == VTERM_PROP_CURSORSHAPE_BAR_LEFT) {
                self->cursor_type = EINK_CURSOR_VERTICAL;
            }
            else {
                self->cursor_type = EINK_CURSOR_HORIZONTAL;
            }

            pos.col = self->cursor_new_col;
            pos.row = self->cursor_new_row;
            movecursor(pos, pos, self->cursor_visible?1:0, user);

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

                if ((cursor_new_col == cursor_old_col + 1) && update_end_col > cursor_new_col && !writer_mode) {
                    //writer_mode = true; No working well with neovim. need
                    //better detection method
                    writer_first_time = false;
                }
                
                // std::cout << writer_first_time << ", " << writer_mode << 
                //     ", " << cursor_new_col << ", " << cursor_old_col << ", " << update_end_col << 
                //     ", " << update_end_row << std::endl;

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

            
            if (should_update) {
                update_start_col = std::min(std::min(update_start_col, cursor_new_col), cursor_old_col);
                update_start_row = std::min(std::min(update_start_row, cursor_new_row), cursor_old_row);
                update_end_col = std::max(std::max(update_end_col, cursor_new_col), cursor_old_col);
                update_end_row = std::max(std::max(update_end_row, cursor_new_row), cursor_old_row);
                eink.refreshRect(update_start_row, 0, update_end_row - update_start_row, eink.cols);
            }
            else if (should_update_cursor){

                int start_col = std::min(cursor_new_col, cursor_old_col);
                int start_row = std::min(cursor_new_row, cursor_old_row);
                int end_col = std::max(cursor_new_col, cursor_old_col);
                int end_row = std::max(cursor_new_row, cursor_old_row);
                eink.refreshRect(start_row, start_col, end_row - start_row + 1, end_col - start_col + 1);

            }

            
            should_update_cursor = false;
            should_update = false;
            update_end_col = 0;
            update_end_row = 0;
            update_start_col = 1337;
            update_start_row = 1337;

            timerfd_settime(timer_fd_dmg, 0, &ts_off, 0);
            timerfd_settime(timer_fd_upd, 0, &ts_upd, 0);
            upd_timer_on = true;
        }

    }
    else if (fd == timer_fd_upd) {
        // std::cout << "UPD timer" << std::endl;

        
        if (should_update || should_update_cursor || writer_mode) {
            bool w;
            w = writer_mode;
            writer_mode = false;
            writer_first_time = true; 
            eink.config.no_refresh = true;

            eink.ot_config.padding = FULL_PADDING;
            
            if (w) {

                update_start_col = 0;
                update_start_row = 0;
                update_end_row = eink.rows;
                update_end_col = eink.cols;
            }

            if (should_update || w) {
                
                updateScreen(update_start_row, update_start_col, 
                        update_end_row - update_start_row, update_end_col - update_start_col);
            }

            if (should_update_cursor || w) {
                // If old cursor is within update region, or cursor doesn move
                // (but might change type), no need to redraw
                // old one since it has been 
                if ((should_update && cursor_old_row >= update_start_row && cursor_old_row < update_end_row &&
                            cursor_old_col >= update_start_col && cursor_old_col < update_end_col) || 
                        (cursor_old_row == cursor_new_row && cursor_old_col == cursor_new_col) || w)
                    updateCursor(cursor_new_row, cursor_new_col, cursor_old_row, cursor_old_col, false); 

                else
                    updateCursor(cursor_new_row, cursor_new_col, cursor_old_row, cursor_old_col, true); 

            }


            eink.config.no_refresh = false;
            eink.refreshRect(0, 0, eink.rows, eink.cols);

            should_update_cursor = false;
            should_update = false;
            update_end_col = 0;
            update_end_row = 0;
            update_start_row = 1337;
            update_start_col = 1337;
            timerfd_settime(timer_fd_dmg, 0, &ts_off, 0);
            timerfd_settime(timer_fd_upd, 0, &ts_upd, 0);
            upd_timer_on = true;

        }
        else {
            timerfd_settime(timer_fd_upd, 0, &ts_off, 0);
            upd_timer_on = false;
        }
        
        writer_first_time = true;

    }

    return true;

}


void Terminal::update_fg_color(VTermColor * c) {
    vterm_screen_convert_color_to_rgb(screen, c);
#define FG(x) static_cast<uint8_t>((x^0xFFu) / 2u)
#define BG(x) static_cast<uint8_t>(x^0xFFu)
    fbink_set_fg_pen_rgba(FG(c->rgb.red), FG(c->rgb.green), FG(c->rgb.blue), 0xFFu, false, true);
}

void Terminal::update_bg_color(VTermColor * c) {
    vterm_screen_convert_color_to_rgb(screen, c);
    fbink_set_bg_pen_rgba(BG(c->rgb.red), BG(c->rgb.green), BG(c->rgb.blue), 0xFFu, false, true);
#undef BG
#undef FG
    }
