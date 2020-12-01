#include "frame_imageview.h"
#include "FS.h"
#include "SPIFFS.h"
#include <HTTPClient.h>

void key_imageview_exit_cb(epdgui_args_vector_t &args)
{
    // EPDGUI_PopFrame(true);
    // *((int*)(args[0])) = 0;
}

Frame_ImageView::Frame_ImageView()
{
    _frame_name = "Frame_ImageView";

    _url = getImageUrl();
    _canvas = new M5EPD_Canvas(&M5.EPD);
    _canvas_download = new M5EPD_Canvas(&M5.EPD);
    _canvas->createCanvas(540, 960);
    _canvas_download->createCanvas(540, 80);

    _canvas_download->setTextSize(26);
    _canvas_download->setTextDatum(CC_DATUM);
    _canvas_download->setTextColor(15);

    _key_exit = new EPDGUI_Button("", 0, 0, 540, 960, EPDGUI_Button::STYLE_INVISABLE);
    _key_exit->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key_exit->Bind(EPDGUI_Button::EVENT_RELEASED, key_imageview_exit_cb);
}

Frame_ImageView::Frame_ImageView(String url)
{
    _frame_name = "Frame_ImageView";

    _url = url;
    _canvas = new M5EPD_Canvas(&M5.EPD);
    _canvas->createCanvas(540, 960);

    _key_exit = new EPDGUI_Button("", 0, 0, 540, 960, EPDGUI_Button::STYLE_INVISABLE);
    _key_exit->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key_exit->Bind(EPDGUI_Button::EVENT_RELEASED, key_imageview_exit_cb);
}

Frame_ImageView::~Frame_ImageView()
{
    delete _canvas;
}

uint8_t *Frame_ImageView::download(String url, uint32_t *psize)
{
    log_d("download from %s", url.c_str());
    HTTPClient http;
    char buf[50];

    _canvas_download->fillCanvas(0);
    _canvas_download->drawString("Connecting...", 270, 40);
    _canvas_download->pushCanvas(0, 440, UPDATE_MODE_GL16);

    _canvas_download->fillCanvas(0);

    if (WiFi.status() != WL_CONNECTED)
    {
        log_e("Not connected");
        _canvas_download->drawString("Wifi not connected.", 270, 40);
        _canvas_download->pushCanvas(0, 440, UPDATE_MODE_GL16);
        return NULL;
    }

    http.begin(url);

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK)
    {
        log_e("HTTP ERROR: %d\n", httpCode);
        http.end();
        sprintf(buf, "HTTP ERROR: %d\n", httpCode);
        _canvas_download->drawString(buf, 270, 40);
        _canvas_download->pushCanvas(0, 440, UPDATE_MODE_GL16);
        http.end();
        return NULL;
    }

    size_t size = http.getSize();

    const uint16_t kBarLength = 400;
    const uint16_t kBarX = (540 - kBarLength) / 2;

    log_d("jpg size = %d Bytes", size);
    sprintf(buf, "0 / %d Bytes", size);
    _canvas_download->drawRect(kBarX, 10, kBarLength, 20, 15);
    _canvas_download->drawString(buf, 270, 60);
    _canvas_download->pushCanvas(0, 440, UPDATE_MODE_GL16);

    *psize = size;
    WiFiClient *stream = http.getStreamPtr();
    uint8_t *p = (uint8_t *)ps_malloc(size);
    if (p == NULL)
    {
        log_e("Memory overflow.");
        http.end();
        return NULL;
    }

    log_d("downloading...");
    size_t offset = 0;
    while (http.connected())
    {
        size_t len = stream->available();

        if(Serial.available())
        {
            http.end();
            free(p);
            return NULL;
        }

        if (!len)
        {
            delay(1);
            continue;
        }

        stream->readBytes(p + offset, len);
        offset += len;
        log_d("%d / %d", offset, size);
        sprintf(buf, "%d / %d Bytes", offset, size);
        float percent = (float)offset / (float)size;
        uint16_t px = kBarLength * percent;
        _canvas_download->fillCanvas(0);
        _canvas_download->drawRect(kBarX, 10, kBarLength, 20, 15);
        _canvas_download->fillRect(kBarX, 10, px, 20, 15);
        _canvas_download->drawString(buf, 270, 60);
        _canvas_download->pushCanvas(0, 440, UPDATE_MODE_GL16);
        if (offset == size)
        {
            break;
        }
    }

    http.end();
    return p;
}

void Frame_ImageView::save(uint8_t* jpg, uint32_t size)
{
    if(jpg == NULL)
    {
        log_e("null jpg pointer");
        return;
    }
    File file = SPIFFS.open("/cache.jpg", FILE_WRITE);
    if (!file)
    {
        log_e("Failed to open file for writing");
        return;
    }
    int i;
    for(i = 0; i < size; i += 512)
    {
        log_d("saving %d / %d", i, size);
        uint32_t written = file.write(jpg + i, 512);
        if(written != 512)
        {
            log_e("SPIFFS write error at %d", i);
            file.close();
            SPIFFS.remove("/cache.jpg");
            break;
        }
        if(Serial.available())
        {
            file.close();
            return;
        }
    }
    i -= 512;
    uint32_t written = file.write(jpg + i, size - i);
    if(written != size - i)
    {
        log_e("SPIFFS write error");
        file.close();
        SPIFFS.remove("/cache.jpg");
        return;
    }
    
    file.close();
    log_d("image saved.");
}

int Frame_ImageView::run()
{
    bool ret = false;
    if(_is_first)
    {
        _is_first = false;
        _canvas->setTextSize(26);
        _canvas->setTextDatum(CC_DATUM);
        _canvas->drawString("Please upload the url to the picture", 270, 480);
        if (SPIFFS.exists("/cache.jpg"))
        {
            _canvas->fillCanvas(0);
            ret = _canvas->drawJpgFile(SPIFFS, "/cache.jpg");
             _canvas->pushCanvas(0, 0, UPDATE_MODE_GC16);
            if(ret == 0)
            {
                SPIFFS.remove("/cache.jpg");
            }
        }
        if ((ret == 0) && _url.length())
        {
            uint32_t size;
            uint8_t *jpg = download(_url, &size);
            if (jpg != NULL)
            {
                _canvas->fillCanvas(0);
                ret = _canvas->drawJpg(jpg, size);
                if(ret == 0)
                {
                    _canvas->drawString("Error decoding image, please try again", 270, 480);
                }
                _canvas->pushCanvas(0, 0, UPDATE_MODE_GC16);
                save(jpg, size);
                free(jpg);
            }
        }
    }

    if (Serial.available())
    {
        M5.EPD.Clear(false);
        M5.EPD.UpdateFull(UPDATE_MODE_GL16);
        String data = Serial.readStringUntil('\n');
        uint32_t size;
        uint8_t *jpg = download(data, &size);
        if (jpg != NULL)
        {
            _canvas->fillCanvas(0);
            ret = _canvas->drawJpg(jpg, size);
            if(ret == 0)
            {
                _canvas->drawString("Error decoding image, please try again", 270, 480);
            }
            _canvas->pushCanvas(0, 0, UPDATE_MODE_GC16);
            save(jpg, size);
            free(jpg);
            _url = data;
            setImageUrl(_url);
            nvs_handle nvs_arg;
            nvs_open("system", NVS_READWRITE, &nvs_arg);
            nvs_set_str(nvs_arg, "url", _url.c_str());
            nvs_commit(nvs_arg);
            nvs_close(nvs_arg);
        }
        return 1;
    }

    M5.update();

    if(M5.BtnP.wasReleased())
    {
        if(_url.length() > 0)
        {
            M5.EPD.Clear(false);
            M5.EPD.UpdateFull(UPDATE_MODE_GL16);
            uint32_t size;
            uint8_t *jpg = download(_url, &size);
            if (jpg != NULL)
            {
                _canvas->fillCanvas(0);
                _canvas->drawJpg(jpg, size);
                _canvas->pushCanvas(0, 0, UPDATE_MODE_GC16);
                save(jpg, size);
                free(jpg);
            }
        }
    }
    return 1;
}

int Frame_ImageView::init(epdgui_args_vector_t &args)
{
    _is_first = true;
    _is_run = 1;
    M5.EPD.Clear();
    // EPDGUI_AddObject(_key_exit);
    return 0;
}
