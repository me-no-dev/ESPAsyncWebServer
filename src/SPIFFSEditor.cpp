#include "SPIFFSEditor.h"
#include <FS.h>

#define EDFS

#ifndef EDFS
 #include "edit.htm.gz.h"
#endif

#ifdef ESP32 
 #define fullName(x) name(x)
#endif

#define SPIFFS_MAXLENGTH_FILEPATH 32
static const char excludeListFile[] PROGMEM = "/.exclude.files";

typedef struct ExcludeListS {
    char *item;
    ExcludeListS *next;
} ExcludeList;

static ExcludeList *excludes = NULL;

static bool matchWild(const char *pattern, const char *testee) {
  const char *nxPat = NULL, *nxTst = NULL;

  while (*testee) {
    if (( *pattern == '?' ) || (*pattern == *testee)){
      pattern++;testee++;
      continue;
    }
    if (*pattern=='*'){
      nxPat=pattern++; nxTst=testee;
      continue;
    }
    if (nxPat){ 
      pattern = nxPat+1; testee=++nxTst;
      continue;
    }
    return false;
  }
  while (*pattern=='*'){pattern++;}  
  return (*pattern == 0);
}

static bool addExclude(const char *item){
    size_t len = strlen(item);
    if(!len){
        return false;
    }
    ExcludeList *e = (ExcludeList *)malloc(sizeof(ExcludeList));
    if(!e){
        return false;
    }
    e->item = (char *)malloc(len+1);
    if(!e->item){
        free(e);
        return false;
    }
    memcpy(e->item, item, len+1);
    e->next = excludes;
    excludes = e;
    return true;
}

static void loadExcludeList(fs::FS &_fs, const char *filename){
    static char linebuf[SPIFFS_MAXLENGTH_FILEPATH];
    fs::File excludeFile=_fs.open(filename, "r");
    if(!excludeFile){
        //addExclude("/*.js.gz");
        return;
    }
#ifdef ESP32
    if(excludeFile.isDirectory()){
      excludeFile.close();
      return;
    }
#endif
    if (excludeFile.size() > 0){
      uint8_t idx;
      bool isOverflowed = false;
      while (excludeFile.available()){
        linebuf[0] = '\0';
        idx = 0;
        int lastChar;
        do {
          lastChar = excludeFile.read();
          if(lastChar != '\r'){
            linebuf[idx++] = (char) lastChar;
          }
        } while ((lastChar >= 0) && (lastChar != '\n') && (idx < SPIFFS_MAXLENGTH_FILEPATH));

        if(isOverflowed){
          isOverflowed = (lastChar != '\n');
          continue;
        }
        isOverflowed = (idx >= SPIFFS_MAXLENGTH_FILEPATH);
        linebuf[idx-1] = '\0';
        if(!addExclude(linebuf)){
            excludeFile.close();
            return;
        }
      }
    }
    excludeFile.close();
}

static bool isExcluded(fs::FS &_fs, const char *filename) {
  if(excludes == NULL){
      loadExcludeList(_fs, String(FPSTR(excludeListFile)).c_str());
  }
  ExcludeList *e = excludes;
  while(e){
    if (matchWild(e->item, filename)){
      return true;
    }
    e = e->next;
  }
  return false;
}

// WEB HANDLER IMPLEMENTATION

#ifdef ESP32
SPIFFSEditor::SPIFFSEditor(const fs::FS& fs, const String& username, const String& password)
#else
SPIFFSEditor::SPIFFSEditor(const String& username, const String& password, const fs::FS& fs)
#endif
:_fs(fs)
,_username(username)
,_password(password)
,_authenticated(false)
,_startTime(0)
{}

bool SPIFFSEditor::canHandle(AsyncWebServerRequest *request){
  if(request->url().equalsIgnoreCase(F("/edit"))){
    if(request->method() == HTTP_GET){
      if(request->hasParam(F("list")))
        return true;
      if(request->hasParam(F("edit"))){
        request->_tempFile = _fs.open(request->arg(F("edit")), "r");
        if(!request->_tempFile){
          return false;
        }
#ifdef ESP32
        if(request->_tempFile.isDirectory()){
          request->_tempFile.close();
          return false;
        }
#endif
      }
      if(request->hasParam("download")){
        request->_tempFile = _fs.open(request->arg(F("download")), "r");
        if(!request->_tempFile){
          return false;
        }
#ifdef ESP32
        if(request->_tempFile.isDirectory()){
          request->_tempFile.close();
          return false;
        }
#endif
      }
      request->addInterestingHeader(F("If-Modified-Since"));
      return true;
    }
    else if(request->method() == HTTP_POST)
      return true;
    else if(request->method() == HTTP_DELETE)
      return true;
    else if(request->method() == HTTP_PUT)
      return true;

  }
  return false;
}


void SPIFFSEditor::handleRequest(AsyncWebServerRequest *request){
  if(_username.length() && _password.length() && !request->authenticate(_username.c_str(), _password.c_str()))
    return request->requestAuthentication();

  if(request->method() == HTTP_GET){
    if(request->hasParam(F("list"))){
      String path = request->getParam(F("list"))->value();
#ifdef ESP32
      File dir = _fs.open(path);
#else
      fs::Dir dir = _fs.openDir(path);
#endif
      path = String();
      String output = "[";
#ifdef ESP32
      File entry = dir.openNextFile();
      while(entry){
#else
      while(dir.next()){
        fs::File entry = dir.openFile("r");
#endif
		String fname = entry.fullName();
		if (fname.charAt(0) != '/') fname = "/" + fname;
		
        if (isExcluded(_fs, fname.c_str())) {
#ifdef ESP32
            entry = dir.openNextFile();
#endif
            continue;
        }
        if (output != "[") output += ',';
        output += F("{\"type\":\"");
        output += F("file");
        output += F("\",\"name\":\"");
        output += String(fname);
        output += F("\",\"size\":");
        output += String(entry.size());
        output += "}";
#ifdef ESP32
        entry = dir.openNextFile();
#else
        entry.close();
#endif
      }
#ifdef ESP32
      dir.close();
#endif
      output += "]";
      request->send(200, F("application/json"), output);
      output = String();
    }
    else if(request->hasParam(F("edit")) || request->hasParam(F("download"))){
      request->send(request->_tempFile, request->_tempFile.fullName(), String(), request->hasParam(F("download")));
    }
    else {
      const char * buildTime = __DATE__ " " __TIME__ " GMT";
      if (request->header(F("If-Modified-Since")).equals(buildTime)) {
        request->send(304);
      } else {
#ifdef EDFS 
         AsyncWebServerResponse *response = request->beginResponse(_fs, F("/edit_gz"), F("text/html"), false);
#else
         AsyncWebServerResponse *response = request->beginResponse_P(200, F("text/html"), edit_htm_gz, edit_htm_gz_len);
#endif
         response->addHeader(F("Content-Encoding"), F("gzip"));
         response->addHeader(F("Last-Modified"), buildTime);
         request->send(response);
      }
    }
  } else if(request->method() == HTTP_DELETE){
    if(request->hasParam(F("path"), true)){
        if(!(_fs.remove(request->getParam(F("path"), true)->value()))){
#ifdef ESP32
			_fs.rmdir(request->getParam(F("path"), true)->value()); // try rmdir for littlefs
#endif
		}			
			
      request->send(200, "", String(F("DELETE: "))+request->getParam(F("path"), true)->value());
    } else
      request->send(404);
  } else if(request->method() == HTTP_POST){
    if(request->hasParam(F("data"), true, true) && _fs.exists(request->getParam(F("data"), true, true)->value()))
      request->send(200, "", String(F("UPLOADED: "))+request->getParam(F("data"), true, true)->value());
  
	else if(request->hasParam(F("rawname"), true) &&  request->hasParam(F("raw0"), true)){
	  String rawnam = request->getParam(F("rawname"), true)->value();
	  
	  if (_fs.exists(rawnam)) _fs.remove(rawnam); // delete it to allow a mode
	  
	  int k = 0;
	  uint16_t i = 0;
	  fs::File f = _fs.open(rawnam, "a");
	  
	  while (request->hasParam(String(F("raw")) + String(k), true)) { //raw0 .. raw1
		if(f){
			i += f.print(request->getParam(String(F("raw")) + String(k), true)->value());  
		}
		k++;
	  }
	  f.close();
	  request->send(200, "", String(F("IPADWRITE: ")) + rawnam + ":" + String(i));
	  
    } else {
      request->send(500);
    }	  
  
  } else if(request->method() == HTTP_PUT){
    if(request->hasParam(F("path"), true)){
      String filename = request->getParam(F("path"), true)->value();
      if(_fs.exists(filename)){
        request->send(200);
      } else {  
/*******************************************************/
#ifdef ESP32  
		if (strchr(filename.c_str(), '/')) {
			// For file creation, silently make subdirs as needed.  If any fail,
			// it will be caught by the real file open later on
			char *pathStr = strdup(filename.c_str());
			if (pathStr) {
				// Make dirs up to the final fnamepart
				char *ptr = strchr(pathStr, '/');
				while (ptr) {
					*ptr = 0;
					_fs.mkdir(pathStr);
					*ptr = '/';
					ptr = strchr(ptr+1, '/');
				}
			}
			free(pathStr);
		}		  
#endif		  
/*******************************************************/		  
        fs::File f = _fs.open(filename, "w");
        if(f){
          f.write((uint8_t)0x00);
          f.close();
          request->send(200, "", String(F("CREATE: "))+filename);
        } else {
          request->send(500);
        }
      }
    } else
      request->send(400);
  }
}

void SPIFFSEditor::handleUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){
  if(!index){
    if(!_username.length() || request->authenticate(_username.c_str(),_password.c_str())){
      _authenticated = true;
      request->_tempFile = _fs.open(filename, "w");
      _startTime = millis();
    }
  }
  if(_authenticated && request->_tempFile){
    if(len){
      request->_tempFile.write(data,len);
    }
    if(final){
      request->_tempFile.close();
    }
  }
}
