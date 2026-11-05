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

// Custom font supporting Swedish characters (Å,Ä,Ö)
LV_FONT_DECLARE(font_montserrat_16);

LilyGo_Class amoled;

// Function: Connects to WIFI
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

// Must have function: Setup is run once on startup
void setup()
{
  Serial.begin(115200);
  delay(200);

  // initialize the AMOLED display hardware and touchscreen controller
  if (!amoled.begin()) {
    Serial.println("Failed to init LilyGO AMOLED.");
    while (true) delay(1000);
  }
  
  beginLvglHelper(amoled);

  // set the default theme with custom font to ensure Å,Ä,Ö renders correctly without becoming a box
  lv_theme_t * th = lv_theme_default_init(
    lv_disp_get_default(), 
    lv_palette_main(LV_PALETTE_BLUE), 
    lv_palette_main(LV_PALETTE_CYAN),
    true, 
    &font_montserrat_16
  );
  lv_disp_set_theme(lv_disp_get_default(), th);

  lv_obj_set_style_text_font(lv_scr_act(), &font_montserrat_16, 0);

  connect_wifi(); // connects wifi
  
  ui_init(); // intializes the entire UI
}

// Must have function: Loop runs continously on device after setup
void loop()
{
  lv_timer_handler();
  delay(5);
}