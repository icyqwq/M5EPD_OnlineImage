#include "frame_imageview.h"

void key_imageview_exit_cb(epdgui_args_vector_t &args)
{
    EPDGUI_PopFrame(true);
    *((int*)(args[0])) = 0;
}

Frame_ImageView::Frame_ImageView(String name)
{
    _frame_name = "Frame_ImageView";

    _name = name;
    _canvas = new M5EPD_Canvas(&M5.EPD);
    _canvas->createCanvas(540, 960);

    _key_exit = new EPDGUI_Button("", 0, 0, 540, 960, EPDGUI_Button::STYLE_INVISABLE);
    _key_exit->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void*)(&_is_run));
    _key_exit->Bind(EPDGUI_Button::EVENT_RELEASED, key_imageview_exit_cb);
}

Frame_ImageView::~Frame_ImageView()
{
    delete _canvas;
}

int Frame_ImageView::init(epdgui_args_vector_t &args)
{
    _is_run = 1;
    M5.EPD.Clear();
    LoadingAnime_32x32_Start(254, 424);
    _canvas->drawJpgUrl(HOST_ADDRESS "/image/" + getMACString() + "/" + _name);
    LoadingAnime_32x32_Stop();
    M5.EPD.WriteFullGram4bpp((uint8_t*)_canvas->frameBuffer());
    EPDGUI_AddObject(_key_exit);
    return 0;
}
