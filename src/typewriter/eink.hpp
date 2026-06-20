#ifndef EINK_H
#define EINK_H

#include <string>
#include <fbink.h>

class EInk {

    public:
        int fd;
        FBInkConfig config = {0};
        FBInkState state = {0};
        FBInkOTConfig ot_config = {0};
        FBInkOTFit ot_fit = {0};
        
        int pad_lr = 20;
        int pad_td = 20;
        int char_width;
        int char_height;
        int cols;
        int rows;
        int offset_t;
        int offset_l;

        bool open();
        bool close();
        bool printState();
        
        
        bool loadFonts(const std::string& font_regular, const std::string& font_bold,
                const std::string& font_italic, const std::string& font_bold_italic);
        bool freeFonts();
        bool calcCharSize();
        bool calculateWindowSize();

        bool printText(int row, int col, const std::string& text);
        bool refreshRect(int row, int col,
                         int height, int width);
        bool clearRect(int row, int col,
                         int height, int width);

};


#endif
