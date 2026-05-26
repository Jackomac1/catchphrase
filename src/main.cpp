#include <Arduino.h>
#include <esp_log.h>
#include "app/App.h"

static App app;

void setup() {
    Serial.begin(115200);
    esp_log_level_set("*", ESP_LOG_INFO);
    app.begin();
}

void loop() {
    app.update(millis());
}
