#include "frame_imagelist.h"
#include "frame_imageview.h"
#include <HTTPClient.h>

#define MAX_ITEM_NUM 10

void key_imagelist_select_cb(epdgui_args_vector_t &args)
{
    EPDGUI_Button *key = (EPDGUI_Button*)(args[1]);
    Frame_Base *frame = new Frame_ImageView(key->GetCustomString());
    EPDGUI_PushFrame(frame);
    *((int*)(args[0])) = 0;
}

Frame_ImageList::Frame_ImageList()
{
    _frame_name = "Frame_ImageList";

    _language = GetLanguage();
    _json_doc = new JsonDoc(16384);

    _canvas_title->fillCanvas(0);
    if(_language == LANGUAGE_JA)
    {
        _canvas_title->drawString("リストが空です", 270, 34);
    }
    else if(_language == LANGUAGE_ZH)
    {
        _canvas_title->drawString("列表为空", 270, 34);
    }
    else
    {
        _canvas_title->drawString("List is empty", 270, 34);
    }
}

Frame_ImageList::~Frame_ImageList()
{
    clearKeys();
}

void Frame_ImageList::clearKeys()
{
    for(int i = 0; i < _keys.size(); i++)
    {
        if(_keys[i] != NULL)
        {
            delete _keys[i];
        }
    }
    _keys.clear();
}

void Frame_ImageList::newImageItem(String name)
{
    const uint16_t kHeight = 80;
    const uint16_t kHeightdiv2 = kHeight >> 1;

    EPDGUI_Button *key = new EPDGUI_Button(4, 28 + (_keys.size() % MAX_ITEM_NUM) * (kHeight + 10), 532, kHeight + 4);
    _keys.push_back(key);

    key->SetEnable(false);
    key->CanvasNormal()->fillCanvas(0);
    key->CanvasNormal()->setTextSize(26);
    key->CanvasNormal()->setTextColor(15);
    key->CanvasNormal()->setTextDatum(CL_DATUM);

    uint16_t len = name.length();
    uint16_t n = 0;
    int charcnt = 0;
    char buf[len];
    memcpy(buf, name.c_str(), len);
    String title = name;
    while (n < len)
    {
        uint16_t unicode = key->CanvasNormal()->decodeUTF8((uint8_t*)buf, &n, len - n);
        if((unicode > 0) && (unicode < 0x7F))
        {
            charcnt++;
        }
        else
        {
            charcnt += 2;
        }
        if(charcnt >= 24)
        {
            title = title.substring(0, n);
            title += "...";
            break;
        }
    }

    key->CanvasNormal()->drawString(title, 72, kHeightdiv2 + 5);
    key->CanvasNormal()->pushImage(15, kHeightdiv2 - 16, 32, 32, ImageResource_item_icon_wallpaper_32x32);
    *(key->CanvasPressed()) = *(key->CanvasNormal());
    key->CanvasPressed()->ReverseColor();
    key->SetCustomString(name);
    key->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, &_is_run);
    key->AddArgs(EPDGUI_Button::EVENT_RELEASED, 1, key);
    key->Bind(EPDGUI_Button::EVENT_RELEASED, key_imagelist_select_cb);
}

esp_err_t Frame_ImageList::registerDevice(void)
{
    HTTPClient http;
    http.begin(HOST_ADDRESS "/register");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpResponseCode = http.POST("mac=" + getMACString());

    if (httpResponseCode != 200)
    {
        log_e("HTTP Request failed. code = %d. payload = %s", httpResponseCode, http.getString());
        return ESP_FAIL;
    }
    http.end();

    return ESP_OK;
}

esp_err_t Frame_ImageList::getImageList()
{
    HTTPClient http;

    LoadingAnime_32x32_Start(254, 424);
    http.begin("http://39.101.205.245/image/" + getMACString());
    int httpResponseCode = http.GET();

    if (httpResponseCode != 200)
    {
        log_e("HTTP Request failed. code = %d. payload = %s", httpResponseCode, http.getString());
        return ESP_FAIL;
    }

    String payload = http.getString();
    http.end();

    DeserializationError err = deserializeJson((*_json_doc), payload);
    if (err)
    {
        log_e("DeserializeJson failed: %s", err.c_str());
        return ESP_FAIL;
    }

    JsonArray array = (*_json_doc)["images"].as<JsonArray>();
    clearKeys();
    for(int i = 0; i < array.size(); i++)
    {
        String name = array[i].as<String>();
        log_d("%s", name.c_str());
        newImageItem(name);
    }
    LoadingAnime_32x32_Stop();

    M5.EPD.Clear();
    EPDGUI_Clear();

    size_t idxmax = _keys.size() > MAX_ITEM_NUM ? MAX_ITEM_NUM : _keys.size();
    for(int i = 0; i < idxmax; i++)
    {
        _keys[i]->SetEnable(true);
        EPDGUI_AddObject(_keys[i]);
    }

    if(array.size() == 0)
    {
        log_d("Empty list");
        _canvas_title->pushCanvas(0, 448, UPDATE_MODE_NONE);
    }
    EPDGUI_Draw(UPDATE_MODE_NONE);
    M5.EPD.UpdateFull(UPDATE_MODE_GC16);

    return ESP_OK;
}

int Frame_ImageList::run()
{
    if(_is_first)
    {
        _current_page = 1;
        _is_first = false;
        getImageList();
        _page_num = (_keys.size() - 1) / MAX_ITEM_NUM + 1;
        return 1;
    }

    M5.update();
    
    if(M5.BtnP.wasReleased())
    {
        _is_first = true;
        getImageList();
    }
    if(M5.BtnR.wasReleased())
    {
        if(_current_page != _page_num)
        {
            for(int i = 0; i < _keys.size(); i++)
            {
                _keys[i]->SetEnable(false);
            }
            _current_page++;
            EPDGUI_Clear();
            int start = (_current_page - 1) * MAX_ITEM_NUM;
            int end = _keys.size() > (_current_page * MAX_ITEM_NUM) ? (_current_page * MAX_ITEM_NUM) : _keys.size();
            for(int i = start; i < end; i++)
            {
                _keys[i]->SetEnable(true);
                EPDGUI_AddObject(_keys[i]);
            }
            M5.EPD.Clear();
            EPDGUI_Draw(UPDATE_MODE_NONE);
            M5.EPD.UpdateFull(UPDATE_MODE_GC16);
        }
    }
    if(M5.BtnL.wasReleased())
    {
        if(_current_page != 1)
        {
            for(int i = 0; i < _keys.size(); i++)
            {
                _keys[i]->SetEnable(false);
            }
            _current_page--;
            EPDGUI_Clear();
            int start = (_current_page - 1) * MAX_ITEM_NUM;
            int end = _keys.size() > (_current_page * MAX_ITEM_NUM) ? (_current_page * MAX_ITEM_NUM) : _keys.size();
            for(int i = start; i < end; i++)
            {
                _keys[i]->SetEnable(true);
                EPDGUI_AddObject(_keys[i]);
            }
            M5.EPD.Clear();
            EPDGUI_Draw(UPDATE_MODE_NONE);
            M5.EPD.UpdateFull(UPDATE_MODE_GC16);
        }
    }

    return 1;
}

int Frame_ImageList::init(epdgui_args_vector_t &args)
{
    _is_first = true;
    _time = 0;
    _is_run = 1;
    M5.EPD.Clear();
    _keys.clear();

    registerDevice();
    
    return 1;
}
