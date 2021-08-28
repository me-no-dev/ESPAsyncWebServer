// WARNING: This webserver has no access control. It servers everything from your ESP32 file system.
// WARNING: This webserver has no access control. It servers everything from your ESP32 file system.
AsyncWebServer * webserver = NULL;
const word webserverServerPort = 80;
const String webserverDefaultname = "/index.html";
// WARNING: This webserver has no access control. It servers everything from your ESP32 file system.
// WARNING: This webserver has no access control. It servers everything from your ESP32 file system.

//*****************************************************************************************************
void setupWebserver(void) {
  webserver = new AsyncWebServer(webserverServerPort);

  /* 
     OK, here is the trick:
        everything falls into the "not found" function. It will search for a matching
        file, if found serve it and if not found call the real notfound function.
  */
  webserver->onNotFound(notFound);
  webserver->begin();
}

//*****************************************************************************************************
//*internals.
//*****************************************************************************************************
// 404
void notFound404(AsyncWebServerRequest *request) {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += (request->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += request->args();
  message += "\n";
  for (uint8_t i = 0; i < request->args(); i++) {
    message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
  }
  message += "\nHeaders: ";
  message += request->headers();
  message += "\n";
  for (uint8_t i = 0; i < request->headers(); i++) {
    message += " " + request->headerName(i) + ": " + request->header(i) + "\n";
  }

  // IE will mindesten 512 Byte, um es anzuzeigen, sonst interne IE-Seite....
  while (message.length() < 512) {
    message += "                         \n";
  }
  request->send(404, "text/plain", message);
}

//*****************************************************************************************************
void notFound(AsyncWebServerRequest *request) {
  String uri = request->url();

  // default name
  if (uri == "/") {
    uri = webserverDefaultname;
  }

  // Are we allowd to send compressed data?
  if (request->hasHeader("ACCEPT-ENCODING")) {
    if (request->header("ACCEPT-ENCODING").indexOf("gzip") != -1) {
      // gzip version exists?
      if (LittleFS.exists(uri + ".gz"))  {
        uri += ".gz";
      }
    }
  }

  if (LittleFS.exists(uri)) {
    request->send(LittleFS, uri, String(), false);
  } else {
    notFound404(request);
  }

  delay(1);
}
