#ifndef _FRAME_IMAGEVIEW_H_
#define _FRAME_IMAGEVIEW_H_

#include "frame_base.h"
#include "../epdgui/epdgui.h"

class Frame_ImageView : public Frame_Base
{
public:
    Frame_ImageView();
    Frame_ImageView(String url);
    ~Frame_ImageView();
    int init(epdgui_args_vector_t &args);
    int run();

private:
    uint8_t* download(String url, uint32_t *psize);
    void save(uint8_t* jpg, uint32_t size);
    M5EPD_Canvas* _canvas;
    M5EPD_Canvas* _canvas_download;
    String _url;
    bool _is_first;
};

#endif //_FRAME_IMAGEVIEW_H_

