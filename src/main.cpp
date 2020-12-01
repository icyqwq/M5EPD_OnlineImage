#include <M5EPD.h>
#include <WiFi.h>
#include "epdgui/epdgui.h"
#include "frame/frame.h"
#include "resources/binaryttf.h"
#include "FS.h"
#include "SPIFFS.h"

// Usage:
// Upload your image to https://sm.ms/ (global) or https://imgchr.com/ (china) 
// and copy Image URL like
// https://s3.ax1x.com/2020/12/01/DWORL6.jpg
// https://i.loli.net/2020/12/01/rTFVGimzWAjChY3.jpg
// then send the url to device through serial port

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

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}


void testFileIO(fs::FS &fs, const char * path){
    Serial.printf("Testing file I/O with %s\r\n", path);

    static uint8_t buf[512];
    size_t len = 0;
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }

    size_t i;
    Serial.print("- writing" );
    uint32_t start = millis();
    for(i=0; i<512; i++){
        if ((i & 0x001F) == 0x001F){
          Serial.print(".");
        }
        uint32_t written = file.write(buf, 512);
        if(written != 512)
        {
            log_d("err");
        }
    }
    Serial.println("");
    uint32_t end = millis() - start;
    Serial.printf(" - %u bytes written in %u ms\r\n", 512 * 512, end);
    file.close();

    file = fs.open(path);
    start = millis();
    end = start;
    i = 0;
    if(file && !file.isDirectory()){
        len = file.size();
        size_t flen = len;
        start = millis();
        Serial.print("- reading" );
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            if ((i++ & 0x001F) == 0x001F){
              Serial.print(".");
            }
            len -= toRead;
        }
        Serial.println("");
        end = millis() - start;
        Serial.printf("- %u bytes read in %u ms\r\n", flen, end);
        file.close();
    } else {
        Serial.println("- failed to open file for reading");
    }
}


void setup()
{
    M5.begin();
    M5.EPD.Clear(true);
    M5.EPD.SetRotation(90);
    M5.TP.SetRotation(90);
    esp_err_t load_err = LoadSetting();

    if(!SPIFFS.begin(1))
    {
        log_e("SPIFFS Mount Failed");
        while(1);
    }
    else
    {
        log_d("size of SPIFFS = %d bytes", SPIFFS.totalBytes());
        listDir(SPIFFS, "/", 0);
        // testFileIO(SPIFFS, "/test.jpg");
    }
    

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
        Frame_Base *frame = new Frame_ImageView();
        EPDGUI_AddFrame("Frame_ImageView", frame);
        EPDGUI_PushFrame(frame);
    }
    LoadingAnime_32x32_Stop();
}

void loop()
{
    EPDGUI_MainLoop();
}