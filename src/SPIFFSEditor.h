#ifndef SPIFFSEditor_H_
#define SPIFFSEditor_H_
#include <ESPAsyncWebServer.h>

/* SPIFFSEditorBase sends the basic ACE Editor from SPIFFS (/.edit.html.gz) instead from PROGMEM
   For backwards compatibility SPISSEditor still uses PROGMEM (edit_htm_gz)
   Subclassing was used, to make sure, that the linker does not link the 4k PROGMEM blob, 
   when using SPIFFSEditorBase.
   When using SPIFFSEditorBase you can do changes on the ACE editor (https://ace.c9.io/) 
   without generating hexcode from the minimized HTML and recompiling.  Just throw newer files to SPIFFS
   
*/

class SPIFFSEditorBase: public AsyncWebHandler {
  protected:
    fs::FS _fs;
    String _username;
    String _password; 
    bool _authenticated;
    uint32_t _startTime;
  public:
#ifdef ESP32
    SPIFFSEditorBase(const fs::FS& fs, const String& username=String(), const String& password=String());
#else
    SPIFFSEditorBase(const String& username=String(), const String& password=String(), const fs::FS& fs=SPIFFS);
#endif
    virtual void finishRequest(AsyncWebServerRequest *request);
    virtual bool canHandle(AsyncWebServerRequest *request) override final;
    virtual void handleRequest(AsyncWebServerRequest *request) override final;
    virtual void handleUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) override final;
    virtual bool isRequestHandlerTrivial() override final {return false;}
};

class SPIFFSEditor: public SPIFFSEditorBase {
  public:
    virtual void finishRequest(AsyncWebServerRequest *request);
#ifdef ESP32
    SPIFFSEditor(const fs::FS& fs, const String& username=String(), const String& password=String());
#else
    SPIFFSEditor(const String& username=String(), const String& password=String(), const fs::FS& fs=SPIFFS);
#endif
};

#endif
