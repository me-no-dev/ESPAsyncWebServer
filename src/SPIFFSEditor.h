#ifndef SPIFFSEditor_H_
#define SPIFFSEditor_H_
#include <ESPAsyncWebServer.h>

#define EXCLUDELIST "/.exclude.files"




class SPIFFSEditor: public AsyncWebHandler {
  private:
    String _username;
    String _password; 
    bool _authenticated;
    uint32_t _startTime;
  public:
    SPIFFSEditor(const String& username=String(), const String& password=String());
    virtual bool canHandle(AsyncWebServerRequest *request) override final;
    virtual void handleRequest(AsyncWebServerRequest *request) override final;
    virtual void handleUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) override final;

};


bool isHidden( const char *filename,  const char *excludeList);
bool matchWild( const char *pattern, const char *testee);



#endif
