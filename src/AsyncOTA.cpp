#include "AsyncOTA.h"
#include "Arduino.h"
#include "stdlib_noniso.h"

#if defined(ESP8266)
    #include "ESP8266WiFi.h"
    #include "ESPAsyncTCP.h"
    #include "flash_hal.h"
    #include "FS.h"
#elif defined(ESP32)
    #include "WiFi.h"
    #include "AsyncTCP.h"
    #include "Update.h"
    #include "esp_int_wdt.h"
    #include "esp_task_wdt.h"
#endif

#include "Hash.h"
#include "FS.h"
#include "elegantWebpage.h"
 

#include <string.h>

AsyncOTA::AsyncOTA(AsyncWebServer& server, const char* username, const char* password) : 
    _server(server),
    _username(username),
    _password(password)
{
}

void AsyncOTA::begin(){
 
    bool _auth_required = strlen(_username) > 0;

    _server.on("/update/identity", HTTP_GET, [&](AsyncWebServerRequest *request){
        if(_auth_required){
            if(!request->authenticate(_username, _password)){
                return request->requestAuthentication();
            }
        }
        char json[41]; // reserve for the exact amount of characters
        uint32_t id;
        #if defined(ESP8266)
            id = ESP.getChipId();
        #elif defined(ESP32)
            id = uint32_t(ESP.getEfuseMac());
        #endif
        sprintf(json,"{\"id\": \"" "%x" "\", \"hardware\": \"ESP8266\"}", id);
        request->send(200, "application/json", json);
    });

    _server.on("/update", HTTP_GET, [&](AsyncWebServerRequest *request){
        if(_auth_required){
            if(!request->authenticate(_username, _password)){
                return request->requestAuthentication();
            }
        }
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", ELEGANT_HTML, ELEGANT_HTML_SIZE);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    _server.on("/update", HTTP_POST, [&](AsyncWebServerRequest *request) {
        if(_auth_required){
            if(!request->authenticate(_username, _password)){
                return request->requestAuthentication();
            }
        }
        // the request handler is triggered after the upload has finished... 
        // create the response, add header, and send response
        AsyncWebServerResponse *response = request->beginResponse((Update.hasError())?500:200, "text/plain", (Update.hasError())?"FAIL":"OK");
        response->addHeader("Connection", "close");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        delay(1000);
        ESP.restart();
    }, [&](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        //Upload handler chunks in data
        if(_auth_required){
            if(!request->authenticate(_username, _password)){
                return request->requestAuthentication();
            }
        }

        if (!index) {
            if(!request->hasParam("MD5", true)) {
                return request->send(400, "text/plain", "MD5 parameter missing");
            }

            if(!Update.setMD5(request->getParam("MD5", true)->value().c_str())) {
                return request->send(400, "text/plain", "MD5 parameter invalid");
            }

            #if defined(ESP8266)
                int cmd = (filename == "filesystem") ? U_FS : U_FLASH;
                Update.runAsync(true);
                size_t fsSize = ((size_t) &_FS_end - (size_t) &_FS_start);
                uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
                if (!Update.begin((cmd == U_FS)?fsSize:maxSketchSpace, cmd)){ // Start with max available size
            #elif defined(ESP32)
                int cmd = (filename == "filesystem") ? U_SPIFFS : U_FLASH;
                if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) { // Start with max available size
            #endif
                Update.printError(Serial);
                return request->send(400, "text/plain", "OTA could not begin");
            }
        }

        // Write chunked data to the free sketch space
        if(len){
            if (Update.write(data, len) != len) {
                return request->send(400, "text/plain", "OTA could not begin");
            }
        }
            
        if (final) { // if the final flag is set then this is the last frame of data
            if (!Update.end(true)) { //true to set the size to the current progress
                Update.printError(Serial);
                return request->send(400, "text/plain", "Could not end OTA");
            }
        }else{
            return;
        }
    });
}

