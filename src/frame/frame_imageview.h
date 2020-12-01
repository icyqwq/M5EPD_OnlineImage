#ifndef _FRAME_IMAGEVIEW_H_
#define _FRAME_IMAGEVIEW_H_

#include "frame_base.h"
#include "../epdgui/epdgui.h"

class Frame_ImageView : public Frame_Base
{
public:
    Frame_ImageView(String name);
    ~Frame_ImageView();
    int init(epdgui_args_vector_t &args);

private:
    M5EPD_Canvas* _canvas;
    String _name;
};

#endif //_FRAME_IMAGEVIEW_H_

