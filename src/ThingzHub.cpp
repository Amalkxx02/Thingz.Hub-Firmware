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

// Public Float Version
void ThingzHub::sendData(const char* name, float value) {
    _sendBinaryFrame(THZ_FLOAT, name, (uint8_t*)&value, 4);
}

// Public Int Version
void ThingzHub::sendData(const char* name, int value) {
    _sendBinaryFrame(THZ_INT, name, (uint8_t*)&value, 4);
}

void ThingzHub::sendData(const char* name, bool value){
    _sendBinaryFrame(THZ_BOOL, name, (uint8_t*)&value, 4);
}
void ThingzHub::sendData(const char* name, float x, float y, float z){
    float arr[3] = {x, y, z};
    _sendBinaryFrame(THZ_VEC3, name, (uint8_t*)arr, 12);
}


void ThingzHub::_sendBinaryFrame(uint8_t dataType, const char* name, const uint8_t* valBytes, uint8_t valLen) {
    if (!_is_initialized || !_client->connected()) return;

    uint8_t nameLen = strlen(name);
    uint16_t payloadSize = 1 + 1 + nameLen + valLen;

    // Send WS Header (Fin + Binary)
    _client->write(0x82);
    _client->write(payloadSize | 0x80); // Mask bit + Len
    
    uint8_t mask[4] = {0x01, 0x02, 0x03, 0x04}; // In prod, use random
    _client->write(mask, 4);

    // Header part of payload (Type + NameLen + Name)
    uint8_t header[2] = {dataType, nameLen};
    
    // Mask and send header
    for(int i=0; i<2; i++) _client->write(header[i] ^ mask[i % 4]);
    for(int i=0; i<nameLen; i++) _client->write(name[i] ^ mask[(i+2) % 4]);
    
    // Mask and send actual data bytes
    for(int i=0; i<valLen; i++) {
        _client->write(valBytes[i] ^ mask[(i + 2 + nameLen) % 4]);
    }
}