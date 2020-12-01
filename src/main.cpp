#include <M5EPD.h>
#include <WiFi.h>
#include "epdgui/epdgui.h"
#include "frame/frame.h"
#include "resources/binaryttf.h"

static const char kPrenderGlyph[77] = {
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', //10
       'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',   //9
            'z', 'x', 'c', 'v', 'b', 'n', 'm',   //7
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', //10
       'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',   //9
            'Z', 'X', 'C', 'V', 'B', 'N', 'M',   //7
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', //10
       '-', '/', ':', ';', '(', ')', '$', '&', '@',   //9
            '_', '\'', '.', ',', '?', '!',   //7
};

esp_err_t connectWiFi()
{
    if((GetWifiSSID().length() < 2) || (GetWifiPassword().length() < 8))
    {
        log_d("no valid wifi config");
        return ESP_FAIL;
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin(GetWifiSSID().c_str(), GetWifiPassword().c_str());
    log_d("Connect to %s", GetWifiSSID().c_str());
    uint32_t t = millis();
    while (1)
    {
        if(millis() - t > 8000)
        {
            WiFi.disconnect();
            return ESP_FAIL;
        }

        if(WiFi.status() == WL_CONNECTED)
        {
            return ESP_OK;
        }
    }
}

void setup()
{
    M5.begin();
    M5.EPD.Clear(true);
    M5.EPD.SetRotation(90);
    M5.TP.SetRotation(90);
    esp_err_t load_err = LoadSetting();

    M5EPD_Canvas canvas(&M5.EPD);
    if(SD.exists("/font.ttf"))
    {
        canvas.loadFont("/font.ttf", SD);
        SetTTFLoaded(true);
    }
    else
    {
        canvas.loadFont(binaryttf, sizeof(binaryttf));
        SetTTFLoaded(false);
    }
    SetLanguage(LANGUAGE_EN);
    canvas.createRender(26, 128);

    LoadingAnime_32x32_Start(254, 424);
    esp_err_t wifi_err = ESP_FAIL;
    if(load_err == ESP_OK)
    {
        wifi_err = connectWiFi();
    }

    if(wifi_err == ESP_FAIL)
    {
        Frame_Base *frame = new Frame_WifiPassword();
        EPDGUI_AddFrame("Frame_WifiPassword", frame);
        log_d("pre rending...");
        canvas.setTextSize(26);
        for(int i = 0; i < 77; i++)
        {
            canvas.preRender(kPrenderGlyph[i]);
        }
        frame = new Frame_WifiScan();
        EPDGUI_AddFrame("Frame_WifiScan",  frame);
        EPDGUI_PushFrame(frame);
    }
    else
    {
        Frame_Base *frame = new Frame_ImageList();
        EPDGUI_AddFrame("Frame_ImageList", frame);
        EPDGUI_PushFrame(frame);
    }
    

    LoadingAnime_32x32_Stop();
}

void loop()
{
    EPDGUI_MainLoop();
}