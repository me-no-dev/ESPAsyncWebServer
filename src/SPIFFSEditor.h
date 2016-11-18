#ifndef SPIFFSEditor_H_
#define SPIFFSEditor_H_
#include <ESPAsyncWebServer.h>

class SPIFFSEditor: public AsyncWebHandler {
  private:
    String _username;
    String _password;
    bool _authenticated;
    uint32_t _startTime;
  public:
    SPIFFSEditor(String username=String(), String password=String());
    virtual bool canHandle(AsyncWebServerRequest *request) override final;
    virtual void handleRequest(AsyncWebServerRequest *request) override final;
    virtual void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) override final;
};

#endif
