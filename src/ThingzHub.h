#ifndef THINGZHUB_H
#define THINGZHUB_H

#include <Arduino.h>
#include <Client.h>

#define REST "https://"
#define WEBSOCKET "ws://"
#define URL "url.com"

enum DATATYPE{
    THZ_INT,
    THZ_FLOAT,
    THZ_BOOL,
    THZ_VEC3
};

enum DEVICETYPE{
    SENSOR,
    ACTUATOR,
    HYBRID
};

class ThingzHub{
    private:
    const char* _url = URL;
    const char* _device_id;
    const char* _device_secret;
    bool _is_initialized = false;
    Client* _client;

    void _sendBinaryFrame(uint8_t dataType, const char* name, const uint8_t* valBytes, uint8_t valLen);

    public:
        ThingzHub();

        bool initialize(Client& client,const char* url, const char* id, const char* secret);

        void registerDevice(const char* device_name,DEVICETYPE dType,DATATYPE vType);

        bool initializeWebSocket(const char* url);

        void sendData(const char* resource_name, int value);
        void sendData(const char* resource_name, float value);
        void sendData(const char* resource_name, bool value);
        void sendData(const char* resource_name, float x, float y, float z);
};

#endif