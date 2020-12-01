#ifndef _FRAME_IMAGELIST_H_
#define _FRAME_IMAGELIST_H_

#include "frame_base.h"
#include "../epdgui/epdgui.h"
#include <vector>
#include <map>
#define ARDUINOJSON_DECODE_UNICODE 1
#include <ArduinoJson.h>

struct DefaultAllocator
{
    void *allocate(size_t n)
    {
        if(psramFound())
        {
            return ps_malloc(n);
        }
        else
        {
            return malloc(n);
        }
    }

    void deallocate(void *p)
    {
        free(p);
    }
};

#define JsonDoc BasicJsonDocument<DefaultAllocator>


class Frame_ImageList : public Frame_Base
{
public:
    Frame_ImageList();
    ~Frame_ImageList();
    int init(epdgui_args_vector_t &args);
    int run();

private:
    void newImageItem(String name);
    void clearKeys();
    esp_err_t getImageList();
    esp_err_t registerDevice();

private:
    bool _is_first;
    std::vector<EPDGUI_Button*> _keys;
    uint8_t _language;
    uint32_t _time;
    int _page_num = 1;
    int _current_page = 1;
    JsonDoc* _json_doc;
};

#endif //_FRAME_IMAGELIST_H_

