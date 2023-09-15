#include "utils.h"
#include <Arduino.h>

void restart() {
    yield();
    delay(1000);
    yield();
    ESP.restart();
}

String getID(){
    String id = "";
    #if defined(ESP8266)
        id = String(ESP.getChipId());
    #elif defined(ESP32)
        id = String((uint32_t)ESP.getEfuseMac(), HEX);
    #endif
    id.toUpperCase();
    return id;
}
