#include "ThingzHub.h"

#define UUID_LENGTH 36

ThingzHub::ThingzHub(){
    _url = nullptr;
    _device_id = nullptr;
    _device_secret = nullptr;
    _is_initialized = false;
    _client = nullptr;
}

bool ThingzHub::initialize(Client& client,const char* url, const char* id, const char* secret){

    if (_is_initialized) return true;

    _url = url;
    _device_id = id;
    _device_secret = secret;
    _client = &client;
    uint8_t id_len = strlen(_device_id);

    if(id_len != UUID_LENGTH){
        Serial.println("ThingzHub Error: Invalid UUID length. Expected 36 chars.");
        return false;
    }

    if(!_client->connect(_url,443)){
        return false;
    }

    _client->println("POST /intitlesize HTTP/1.1");
    _client->print("Host: "); _client->println(_url);
    _client->print("Authorization: Bearer "); _client->println(_device_secret);
    _client->println("Content-Type: application/json");
    uint8_t length = 9 + UUID_LENGTH;
    _client->print("Content-Length: "); _client->println(length);
    _client->println();
    _client->print("{\"id\":\""); _client->print(_device_id); _client->println("\"}");
    unsigned long taskTime = millis();
    while(!_client->available()){
        if(millis() - taskTime > 5000) return false;
    }

    for(uint8_t i = 0; i < 9; i++){
        while(!_client->available())
        _client->read();
    }

    uint8_t code[3] = {0};
    for(uint8_t i = 0; i < 3; i++){
        while(!_client->available())
        code[i] = _client->read();
    }

    if (code[0] == '2' && code[1] == '0' && code[2] == '0') {
        _is_initialized = true;
        _client->stop();
        return true;

    }

    _is_initialized = false;
    _client->stop();
    return false;
};

void ThingzHub::registerDevice(const char* device_name, DEVICETYPE dType, DATATYPE vType) {
    if (!_is_initialized) return;

    if (_client->connect(_url, 80)) {
        _client->println("POST /register HTTP/1.1");
        _client->print("Host: "); _client->println(_url);
        _client->print("Authorization: Bearer "); _client->println(_device_secret);
        _client->println("Content-Type: application/json");
        
        _client->print("Content-Length: "); 
        _client->println(25 + strlen(device_name)); 
        _client->println();
        
        _client->print("{\"name\":\""); _client->print(device_name);
        _client->print("\",\"type\":"); _client->print((int)dType);
        _client->print(",\"data\":"); _client->print((int)vType);
        _client->println("}");
        
        _client->stop();
    }
}

bool ThingzHub::initializeWebSocket(const char* url) {
    if (!_is_initialized) return false;

    if (!_client->connect(_url, 80)) { 
        return false;
    }

    _client->println("GET /ws HTTP/1.1");
    _client->print("Host: "); _client->println(_url);
    _client->println("Upgrade: websocket");
    _client->println("Connection: Upgrade");
    
    _client->print("X-Device-ID: "); _client->println(_device_id);
    _client->print("Authorization: Bearer "); _client->println(_device_secret);
    
    _client->println("Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ=="); 
    _client->println("Sec-WebSocket-Version: 13");
    _client->println();
    unsigned long start = millis();
    while(_client->available() < 12) {
        if(millis() - start > 5000) return false;
    }
    
    for(int i=0; i<9; i++) _client->read(); 
    char code[3];
    for(int i=0; i<3; i++) code[i] = _client->read();

    if(code[0] == '1' && code[1] == '0' && code[2] == '1') {
        return true; 
    }

    return false;
}