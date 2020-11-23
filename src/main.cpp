#include <M5EPD.h>
#include <WiFi.h>

// Example of displaying online images
// Drawing png will consume a lot of time
// Drawing jpg needs to consume the corresponding heap space to download the image

M5EPD_Canvas canvas(&M5.EPD);

esp_err_t connectWiFi(String ssid, String pswd)
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pswd.c_str());
    log_d("Connect to %s", ssid.c_str());
    uint32_t t = millis();
    while (1)
    {
        if(millis() - t > 8000)
        {
            WiFi.disconnect();
            log_e("Connect timeout.");
            return ESP_FAIL;
        }

        if(WiFi.status() == WL_CONNECTED)
        {
            return ESP_OK;
        }
    }
}

void malloc_test(size_t size)
{
    uint8_t* p = NULL;
    p = (uint8_t*)ps_malloc(size);
    if(p != NULL)
    {
        free(p);
        log_d("malloc %d SUCCESS", size);
    }
    else
    {
        log_d("malloc %d FAILED", size);
    }
}

void setup()
{
    M5.begin();
    M5.EPD.Clear(true);
    M5.EPD.SetRotation(90);
    M5.TP.SetRotation(90);

    connectWiFi("M5-5G", "Office@888888");

    canvas.createCanvas(540, 960);
    canvas.drawJpgUrl("https://cdn.shopifycdn.net/s/files/1/0056/7689/2250/files/LOGO_60x@2x.jpg?v=1563273356");
    canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
}

void loop()
{
    // put your main code here, to run repeatedly:
}