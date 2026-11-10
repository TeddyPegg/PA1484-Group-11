# 1 "C:\\Users\\noahs\\AppData\\Local\\Temp\\tmpv_z9a7oy"
#include <Arduino.h>
# 1 "C:/Users/noahs/Desktop/BTH2026/ESP32 software grupp 11/PA1484-Group-11/project/project.ino"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <time.h>
#include <LilyGo_AMOLED.h>
#include <LV_Helper.h>
#include <lvgl.h>

#include "secrets.h"
#include "timetable.h"
#include "ui_manager.h"
#include "header.h"


LV_FONT_DECLARE(font_montserrat_16);

LilyGo_Class amoled;
static void connect_wifi();
void setup();
void loop();
#line 22 "C:/Users/noahs/Desktop/BTH2026/ESP32 software grupp 11/PA1484-Group-11/project/project.ino"
static void connect_wifi()
{
  Serial.printf("Connecting to WiFi SSID: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000) {
    delay(250);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected.");
  } else {
    Serial.println("WiFi could not connect (timeout).");
  }
}


void setup()
{
  Serial.begin(115200);
  delay(200);


  if (!amoled.begin()) {
    Serial.println("Failed to init LilyGO AMOLED.");
    while (true) delay(1000);
  }

  beginLvglHelper(amoled);


  lv_theme_t * th = lv_theme_default_init(
    lv_disp_get_default(),
    lv_palette_main(LV_PALETTE_BLUE),
    lv_palette_main(LV_PALETTE_CYAN),
    true,
    &font_montserrat_16
  );
  lv_disp_set_theme(lv_disp_get_default(), th);

  lv_obj_set_style_text_font(lv_scr_act(), &font_montserrat_16, 0);

  connect_wifi();

  ui_init();
}


void loop()
{
  lv_timer_handler();
  delay(5);
}