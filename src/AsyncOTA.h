#ifndef ASYNC_OTA_H_
#define ASYNC_OTA_H_

#include <ESPAsyncWebServer.h>

class AsyncOTA{

    public:
        AsyncOTA(AsyncWebServer& server, const char* username="", const char* password="");
        void begin();

    private:
        AsyncWebServer& _server;
        const char* _username = "";
        const char* _password = "";
};
 
#endif
