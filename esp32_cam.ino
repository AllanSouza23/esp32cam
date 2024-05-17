#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>

#include "src/OV2640Streamer.h"
#include "src/CRtspSession.h"
#include "src/OV2640.h"

#include "camera_pins.h"
#include "wifikeys.h"

OV2640 cam;
WiFiServer rtspServer(8554);

void setup()
{
    Serial.begin(115200);
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 5; 
    config.fb_count = 2;       
    cam.init(config);
    
    IPAddress ip;

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(F("."));
    }
    ip = WiFi.localIP();
    Serial.println(F("WiFi connected"));
    Serial.println("");
    Serial.println(ip);
    Serial.print("Stream Link: rtsp://");
    Serial.print(ip);
    Serial.println(":8554/mjpeg/1");
    rtspServer.begin();
}

CStreamer *streamer;
CRtspSession *session;
WiFiClient client;

void loop()
{
    uint32_t msecPerFrame = 100;
    static uint32_t lastimage = millis();

    if(session) {
        session->handleRequests(0);

        uint32_t now = millis();
        if(now > lastimage + msecPerFrame || now < lastimage) {
            session->broadcastCurrentFrame(now);
            lastimage = now;

            now = millis();
            if(now > lastimage + msecPerFrame)
                printf("warning exceeding max frame rate of %d ms\n", now - lastimage);
        }

        if(session->m_stopped) {
            delete session;
            delete streamer;
            session = NULL;
            streamer = NULL;
        }
    }
    else {
        client = rtspServer.accept();

        if(client) {
            streamer = new OV2640Streamer(&client, cam);
            session = new CRtspSession(&client, streamer);
        }
    }
}
