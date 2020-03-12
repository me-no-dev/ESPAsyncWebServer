/**************************************************************
   AsyncWiFiManager is a library for the ESP8266/Arduino platform
   (https://github.com/esp8266/Arduino) to enable easy
   configuration and reconfiguration of WiFi credentials using a Captive Portal
   inspired by:
   http://www.esp8266.com/viewtopic.php?f=29&t=2520
   https://github.com/chriscook8/esp-arduino-apboot
   https://github.com/esp8266/Arduino/tree/esp8266/hardware/esp8266com/esp8266/libraries/DNSServer/examples/CaptivePortalAdvanced
   Built by AlexT https://github.com/tzapu
   Ported to Async Web Server by https://github.com/alanswx
   Licensed under MIT license
 **************************************************************/

#include "ESPAsyncWiFiManager.h"

AsyncWiFiManagerParameter::AsyncWiFiManagerParameter(const char *custom) {
  _id = NULL;
  _placeholder = NULL;
  _length = 0;
  _value = NULL;

  _customHTML = custom;
}

AsyncWiFiManagerParameter::AsyncWiFiManagerParameter(const char *id, const char *placeholder, const char *defaultValue, int length) {
  init(id, placeholder, defaultValue, length, "");
}

AsyncWiFiManagerParameter::AsyncWiFiManagerParameter(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom) {
  init(id, placeholder, defaultValue, length, custom);
}

void AsyncWiFiManagerParameter::init(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom) {
  _id = id;
  _placeholder = placeholder;
  _length = length;
  _value = new char[length + 1];
  for (int i = 0; i < length; i++) {
    _value[i] = 0;
  }
  if (defaultValue != NULL) {
    strncpy(_value, defaultValue, length);
  }

  _customHTML = custom;
}

const char* AsyncWiFiManagerParameter::getValue() {
  return _value;
}
const char* AsyncWiFiManagerParameter::getID() {
  return _id;
}
const char* AsyncWiFiManagerParameter::getPlaceholder() {
  return _placeholder;
}
int AsyncWiFiManagerParameter::getValueLength() {
  return _length;
}
const char* AsyncWiFiManagerParameter::getCustomHTML() {
  return _customHTML;
}

#ifdef USE_EADNS
AsyncWiFiManager::AsyncWiFiManager(AsyncWebServer *server, AsyncDNSServer *dns) :server(server), dnsServer(dns) {
#else
AsyncWiFiManager::AsyncWiFiManager(AsyncWebServer *server, DNSServer *dns) :server(server), dnsServer(dns) {
#endif
  wifiSSIDs = NULL;
  wifiSSIDscan=true;
  _modeless=false;
  shouldscan=true;
}

void AsyncWiFiManager::addParameter(AsyncWiFiManagerParameter *p) {
  _params[_paramsCount] = p;
  _paramsCount++;
  DEBUG_WM("Adding parameter");
  DEBUG_WM(p->getID());
}

void AsyncWiFiManager::setupConfigPortal() {
  // dnsServer.reset(new DNSServer());
  // server.reset(new ESP8266WebServer(80));
  server->reset();

  DEBUG_WM(F(""));
  _configPortalStart = millis();

  DEBUG_WM(F("Configuring access point... "));
  DEBUG_WM(_apName);
  if (_apPassword != NULL) {
    if (strlen(_apPassword) < 8 || strlen(_apPassword) > 63) {
      // fail passphrase to short or long!
      DEBUG_WM(F("Invalid AccessPoint password. Ignoring"));
      _apPassword = NULL;
    }
    DEBUG_WM(_apPassword);
  }

  //optional soft ip config
  if (_ap_static_ip) {
    DEBUG_WM(F("Custom AP IP/GW/Subnet"));
    WiFi.softAPConfig(_ap_static_ip, _ap_static_gw, _ap_static_sn);
  }

  if (_apPassword != NULL) {
    WiFi.softAP(_apName, _apPassword);//password option
  } else {
    WiFi.softAP(_apName);
  }

  delay(500); // Without delay I've seen the IP address blank
  DEBUG_WM(F("AP IP address: "));
  DEBUG_WM(WiFi.softAPIP());

  /* Setup the DNS server redirecting all the domains to the apIP */
  #ifdef USE_EADNS
  dnsServer->setErrorReplyCode(AsyncDNSReplyCode::NoError);
  #else
  dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
  #endif
  dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());

  setInfo();

  /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  server->on("/", std::bind(&AsyncWiFiManager::handleRoot, this,std::placeholders::_1)).setFilter(ON_AP_FILTER);
  server->on("/wifi", std::bind(&AsyncWiFiManager::handleWifi, this, std::placeholders::_1,true)).setFilter(ON_AP_FILTER);
  server->on("/0wifi", std::bind(&AsyncWiFiManager::handleWifi, this,std::placeholders::_1, false)).setFilter(ON_AP_FILTER);
  server->on("/wifisave", std::bind(&AsyncWiFiManager::handleWifiSave,this,std::placeholders::_1)).setFilter(ON_AP_FILTER);
  server->on("/i", std::bind(&AsyncWiFiManager::handleInfo,this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
  server->on("/r", std::bind(&AsyncWiFiManager::handleReset, this,std::placeholders::_1)).setFilter(ON_AP_FILTER);
  //server->on("/generate_204", std::bind(&AsyncWiFiManager::handle204, this));  //Android/Chrome OS captive portal check.
  server->on("/fwlink", std::bind(&AsyncWiFiManager::handleRoot, this,std::placeholders::_1)).setFilter(ON_AP_FILTER);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server->onNotFound (std::bind(&AsyncWiFiManager::handleNotFound,this,std::placeholders::_1));
  server->begin(); // Web server start
  DEBUG_WM(F("HTTP server started"));

}

static const char HEX_CHAR_ARRAY[17] = "0123456789ABCDEF";
/**
* convert char array (hex values) to readable string by seperator
* buf:           buffer to convert
* length:        data length
* strSeperator   seperator between each hex value
* return:        formated value as String
*/
static String byteToHexString(uint8_t* buf, uint8_t length, String strSeperator="-") {
  String dataString = "";
  for (uint8_t i = 0; i < length; i++) {
    byte v = buf[i] / 16;
    byte w = buf[i] % 16;
    if (i>0) {
      dataString += strSeperator;
    }
    dataString += String(HEX_CHAR_ARRAY[v]);
    dataString += String(HEX_CHAR_ARRAY[w]);
  }
  dataString.toUpperCase();
  return dataString;
} // byteToHexString

#if !defined(ESP8266)
String getESP32ChipID() {
  uint64_t chipid;
  chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
  int chipid_size = 6;
  uint8_t chipid_arr[chipid_size];
  for (uint8_t i=0; i < chipid_size; i++) {
    chipid_arr[i] = (chipid >> (8 * i)) & 0xff;
  }
  return byteToHexString(chipid_arr, chipid_size, "");
}
#endif

boolean AsyncWiFiManager::autoConnect(unsigned long maxConnectRetries, unsigned long retryDelayMs) {
  String ssid = "ESP";
  #if defined(ESP8266)
  ssid += String(ESP.getChipId());
  #else
  ssid += getESP32ChipID();
  #endif
  return autoConnect(ssid.c_str(), NULL);
}

boolean AsyncWiFiManager::autoConnect(char const *apName, char const *apPassword, unsigned long maxConnectRetries, unsigned long retryDelayMs) {
  DEBUG_WM(F(""));

  // read eeprom for ssid and pass
  //String ssid = getSSID();
  //String pass = getPassword();

  // attempt to connect; should it fail, fall back to AP
  WiFi.mode(WIFI_STA);

  for(unsigned long tryNumber = 0; tryNumber < maxConnectRetries; tryNumber++) {
	  DEBUG_WM(F("AutoConnect Try No.:"));
	  DEBUG_WM(tryNumber);

	  if (connectWifi("", "") == WL_CONNECTED)   {
		  DEBUG_WM(F("IP Address:"));
		  DEBUG_WM(WiFi.localIP());
		  //connected
		  return true;
	  }

	  if(tryNumber + 1 < maxConnectRetries) {

		  // we might connect during the delay
		  unsigned long restDelayMs = retryDelayMs;
		  while(restDelayMs != 0) {
			  if(WiFi.status() == WL_CONNECTED) {
				  DEBUG_WM(F("IP Address (connected during delay):"));
				  DEBUG_WM(WiFi.localIP());
				  return true;
			  }
			  unsigned long thisDelay = std::min(restDelayMs, 100ul);
			  delay(thisDelay);
			  restDelayMs -= thisDelay;
		  }

	  }
  }


  return startConfigPortal(apName, apPassword);
}


String AsyncWiFiManager::networkListAsString()
{
  String pager ;
  //display networks in page
  for (int i = 0; i < wifiSSIDCount; i++) {
    if (wifiSSIDs[i].duplicate == true) continue; // skip dups
    int quality = getRSSIasQuality(wifiSSIDs[i].RSSI);

    if (_minimumQuality == -1 || _minimumQuality < quality) {
      String item = FPSTR(HTTP_ITEM);
      String rssiQ;
      rssiQ += quality;
      item.replace("{v}", wifiSSIDs[i].SSID);
      item.replace("{r}", rssiQ);
#if defined(ESP8266)
      if (wifiSSIDs[i].encryptionType != ENC_TYPE_NONE) {
#else
      if (wifiSSIDs[i].encryptionType != WIFI_AUTH_OPEN) {
#endif
        item.replace("{i}", "l");
      } else {
        item.replace("{i}", "");
      }
      pager += item;

    } else {
      DEBUG_WM(F("Skipping due to quality"));
    }

  }
  return pager;
}

String AsyncWiFiManager::scanModal()
{
  shouldscan=true;
  scan();
  String pager=networkListAsString();
  return pager;
}

void AsyncWiFiManager::scan()
{
  if (!shouldscan) return;
  DEBUG_WM(F("About to scan()"));
  if (wifiSSIDscan)
  {
    delay(100);
  }

  if (wifiSSIDscan)
  {
    wifi_ssid_count_t n = WiFi.scanNetworks();
    DEBUG_WM(F("Scan done"));
	if(n == WIFI_SCAN_FAILED) {
      DEBUG_WM(F("scanNetworks returned: WIFI_SCAN_FAILED!"));
	} else if(n == WIFI_SCAN_RUNNING) {
      DEBUG_WM(F("scanNetworks returned: WIFI_SCAN_RUNNING!"));
	} else if(n < 0) {
      DEBUG_WM(F("scanNetworks failed with unknown error code!"));
	} else if (n == 0) {
      DEBUG_WM(F("No networks found"));
      // page += F("No networks found. Refresh to scan again.");
    } else {


      if (wifiSSIDscan)
      {
        /* WE SHOULD MOVE THIS IN PLACE ATOMICALLY */
        if (wifiSSIDs) delete [] wifiSSIDs;
        wifiSSIDs = new WiFiResult[n];
        wifiSSIDCount = n;

        if (n>0)
        shouldscan=false;

        for (wifi_ssid_count_t i=0;i<n;i++)
        {
          wifiSSIDs[i].duplicate=false;

#if defined(ESP8266)
          bool res=WiFi.getNetworkInfo(i, wifiSSIDs[i].SSID, wifiSSIDs[i].encryptionType, wifiSSIDs[i].RSSI, wifiSSIDs[i].BSSID, wifiSSIDs[i].channel, wifiSSIDs[i].isHidden);
#else
          bool res=WiFi.getNetworkInfo(i, wifiSSIDs[i].SSID, wifiSSIDs[i].encryptionType, wifiSSIDs[i].RSSI, wifiSSIDs[i].BSSID, wifiSSIDs[i].channel);
#endif
        }


        // RSSI SORT

        // old sort
        for (int i = 0; i < n; i++) {
          for (int j = i + 1; j < n; j++) {
            if (wifiSSIDs[j].RSSI > wifiSSIDs[i].RSSI) {
              std::swap(wifiSSIDs[i], wifiSSIDs[j]);
            }
          }
        }


        // remove duplicates ( must be RSSI sorted )
        if (_removeDuplicateAPs) {
          String cssid;
          for (int i = 0; i < n; i++) {
            if (wifiSSIDs[i].duplicate == true) continue;
            cssid = wifiSSIDs[i].SSID;
            for (int j = i + 1; j < n; j++) {
              if (cssid == wifiSSIDs[j].SSID) {
                DEBUG_WM("DUP AP: " +wifiSSIDs[j].SSID);
                wifiSSIDs[j].duplicate=true; // set dup aps to NULL
              }
            }
          }
        }

      }
    }
  }
}


void AsyncWiFiManager::startConfigPortalModeless(char const *apName, char const *apPassword) {

  _modeless =true;
  _apName = apName;
  _apPassword = apPassword;

  /*
  AJS - do we want this?

  */

  //setup AP
  WiFi.mode(WIFI_AP_STA);
  DEBUG_WM("SET AP STA");

  // try to connect
  if (connectWifi("", "") == WL_CONNECTED)   {
    DEBUG_WM(F("IP Address:"));
    DEBUG_WM(WiFi.localIP());
    //connected
    // call the callback!
	if ( _savecallback != NULL) {
	  //todo: check if any custom parameters actually exist, and check if they really changed maybe
	  _savecallback();
	}
  }



  //notify we entered AP mode
  if ( _apcallback != NULL) {
    _apcallback(this);
  }

  connect = false;
  setupConfigPortal();
  scannow= -1 ;

}

void AsyncWiFiManager::loop(){
	safeLoop();
	criticalLoop();
}

void AsyncWiFiManager::setInfo() {
	if (needInfo) {
		pager = infoAsString();
	    wifiStatus = WiFi.status();
	    needInfo = false;
	}
}

/**
 * Anything that accesses WiFi, ESP or EEPROM goes here
 */
void AsyncWiFiManager::criticalLoop(){
  if (_modeless)
  {

	if ( scannow==-1 || millis() > scannow + 60000)
	{
	  scan();
	  scannow= millis() ;
	}
	if (connect) {
		connect = false;
	  //delay(2000);
	  DEBUG_WM(F("Connecting to new AP"));

	  // using user-provided  _ssid, _pass in place of system-stored ssid and pass
	  if (connectWifi(_ssid, _pass) != WL_CONNECTED) {
		DEBUG_WM(F("Failed to connect."));
	  } else {
		//connected
		// alanswx - should we have a config to decide if we should shut down AP?
		// WiFi.mode(WIFI_STA);
		//notify that configuration has changed and any optional parameters should be saved
		if ( _savecallback != NULL) {
		  //todo: check if any custom parameters actually exist, and check if they really changed maybe
		  _savecallback();
		}

		return;
	  }

	  if (_shouldBreakAfterConfig) {
		//flag set to exit after config after trying to connect
		//notify that configuration has changed and any optional parameters should be saved
		if ( _savecallback != NULL) {
		  //todo: check if any custom parameters actually exist, and check if they really changed maybe
		  _savecallback();
		}
	  }
	}
  }
}

/*
 * Anything that doesn't access WiFi, ESP or EEPROM can go here
 */
void AsyncWiFiManager::safeLoop(){
  #ifndef USE_EADNS	
  dnsServer->processNextRequest();
  #endif
}

boolean  AsyncWiFiManager::startConfigPortal(char const *apName, char const *apPassword) {
  //setup AP
  WiFi.mode(WIFI_AP_STA);
  DEBUG_WM("SET AP STA");

  _apName = apName;
  _apPassword = apPassword;

  //notify we entered AP mode
  if ( _apcallback != NULL) {
    _apcallback(this);
  }

  connect = false;
  setupConfigPortal();
  scannow= -1 ;
  while (_configPortalTimeout == 0 || millis() < _configPortalStart + _configPortalTimeout) {
    //DNS
    #ifndef USE_EADNS	
    dnsServer->processNextRequest();
    #endif
	
    //
    //  we should do a scan every so often here and
    //  try to reconnect to AP while we are at it
    //
    if ( scannow == -1 || millis() > scannow + 10000)
    {
      DEBUG_WM(F("About to scan()"));
      shouldscan=true;  // since we are modal, we can scan every time
    #if defined(ESP8266)
      // we might still be connecting, so that has to stop for scanning
      ETS_UART_INTR_DISABLE ();
      wifi_station_disconnect ();
      ETS_UART_INTR_ENABLE ();
    #else
      WiFi.disconnect (false);
    #endif
      scan();
      if(_tryConnectDuringConfigPortal) WiFi.begin(); // try to reconnect to AP
      scannow= millis() ;
    }

	// attempts to reconnect were successful
	if(WiFi.status() == WL_CONNECTED) {
		//connected
		WiFi.mode(WIFI_STA);
		//notify that configuration has changed and any optional parameters should be saved
		if ( _savecallback != NULL) {
			//todo: check if any custom parameters actually exist, and check if they really changed maybe
			_savecallback();
		}
		break;
	}


    if (connect) {
      connect = false;
      delay(2000);
      DEBUG_WM(F("Connecting to new AP"));

      // using user-provided  _ssid, _pass in place of system-stored ssid and pass
      if (connectWifi(_ssid, _pass) == WL_CONNECTED) {
        //connected
        WiFi.mode(WIFI_STA);
        //notify that configuration has changed and any optional parameters should be saved
        if ( _savecallback != NULL) {
          //todo: check if any custom parameters actually exist, and check if they really changed maybe
          _savecallback();
        }
        break;
      } else {
        DEBUG_WM(F("Failed to connect."));
      }

      if (_shouldBreakAfterConfig) {
        //flag set to exit after config after trying to connect
        //notify that configuration has changed and any optional parameters should be saved
        if ( _savecallback != NULL) {
          //todo: check if any custom parameters actually exist, and check if they really changed maybe
          _savecallback();
        }
        break;
      }
    }
    yield();
  }

  server->reset();
  #ifdef USE_EADNS
  *dnsServer=AsyncDNSServer();
  #else
  *dnsServer=DNSServer();
  #endif

  return  WiFi.status() == WL_CONNECTED;
}


int AsyncWiFiManager::connectWifi(String ssid, String pass) {
  DEBUG_WM(F("Connecting as wifi client..."));

  // check if we've got static_ip settings, if we do, use those.
  if (_sta_static_ip) {
    DEBUG_WM(F("Custom STA IP/GW/Subnet/DNS"));
    WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn, _sta_static_dns1, _sta_static_dns2);
    DEBUG_WM(WiFi.localIP());
  }
  //fix for auto connect racing issue
  //  if (WiFi.status() == WL_CONNECTED) {
  //    DEBUG_WM("Already connected. Bailing out.");
  //    return WL_CONNECTED;
  //  }
  //check if we have ssid and pass and force those, if not, try with last saved values
  if (ssid != "") {
  #if defined(ESP8266)
    //trying to fix connection in progress hanging
    ETS_UART_INTR_DISABLE();
    wifi_station_disconnect();
    ETS_UART_INTR_ENABLE();
  #else
    WiFi.disconnect(false);
  #endif

    WiFi.begin(ssid.c_str(), pass.c_str());
  } else {

    if (WiFi.SSID().length() > 0) {
      DEBUG_WM("Using last saved values, should be faster");
#if defined(ESP8266)
      //trying to fix connection in progress hanging
      ETS_UART_INTR_DISABLE();
      wifi_station_disconnect();
      ETS_UART_INTR_ENABLE();
#else
      WiFi.disconnect(false);
#endif

      WiFi.begin();
    } else {
      DEBUG_WM("Try to connect with saved credentials");
      WiFi.begin();
    }
  }

  int connRes = waitForConnectResult();
  DEBUG_WM ("Connection result: ");
  DEBUG_WM ( connRes );
  //not connected, WPS enabled, no pass - first attempt
#ifdef NO_EXTRA_4K_HEAP	
  if (_tryWPS && connRes != WL_CONNECTED && pass == "") {
    startWPS();
    //should be connected at the end of WPS
    connRes = waitForConnectResult();
  }
#endif
  needInfo = true;
  setInfo();
  return connRes;
}

uint8_t AsyncWiFiManager::waitForConnectResult() {
  if (_connectTimeout == 0) {
    return WiFi.waitForConnectResult();
  } else {
    DEBUG_WM (F("Waiting for connection result with time out"));
    unsigned long start = millis();
    boolean keepConnecting = true;
    uint8_t status;
    while (keepConnecting) {
      status = WiFi.status();
      if (millis() > start + _connectTimeout) {
        keepConnecting = false;
        DEBUG_WM (F("Connection timed out"));
      }
      if (status == WL_CONNECTED || status == WL_CONNECT_FAILED) {
        keepConnecting = false;
      }
      delay(100);
    }
    return status;
  }
}
#ifdef NO_EXTRA_4K_HEAP
void AsyncWiFiManager::startWPS() {
  DEBUG_WM("START WPS");
#if defined(ESP8266)
  WiFi.beginWPSConfig();
#else
  //esp_wps_config_t config = WPS_CONFIG_INIT_DEFAULT(ESP_WPS_MODE);
  esp_wps_config_t config = {};
  config.wps_type = ESP_WPS_MODE;
  config.crypto_funcs = &g_wifi_default_wps_crypto_funcs;
  strcpy(config.factory_info.manufacturer,"ESPRESSIF");  
  strcpy(config.factory_info.model_number, "ESP32");  
  strcpy(config.factory_info.model_name, "ESPRESSIF IOT");  
  strcpy(config.factory_info.device_name,"ESP STATION");  

  esp_wifi_wps_enable(&config);
  esp_wifi_wps_start(0);
#endif
  DEBUG_WM("END WPS");

}
#endif
/*
String AsyncWiFiManager::getSSID() {
if (_ssid == "") {
DEBUG_WM(F("Reading SSID"));
_ssid = WiFi.SSID();
DEBUG_WM(F("SSID: "));
DEBUG_WM(_ssid);
}
return _ssid;
}

String AsyncWiFiManager::getPassword() {
if (_pass == "") {
DEBUG_WM(F("Reading Password"));
_pass = WiFi.psk();
DEBUG_WM("Password: " + _pass);
//DEBUG_WM(_pass);
}
return _pass;
}
*/
String AsyncWiFiManager::getConfigPortalSSID() {
  return _apName;
}

void AsyncWiFiManager::resetSettings() {
  DEBUG_WM(F("settings invalidated"));
  DEBUG_WM(F("THIS MAY CAUSE AP NOT TO START UP PROPERLY. YOU NEED TO COMMENT IT OUT AFTER ERASING THE DATA."));
  WiFi.disconnect(true);
  //delay(200);
}
void AsyncWiFiManager::setTimeout(unsigned long seconds) {
  setConfigPortalTimeout(seconds);
}

void AsyncWiFiManager::setConfigPortalTimeout(unsigned long seconds) {
  _configPortalTimeout = seconds * 1000;
}

void AsyncWiFiManager::setConnectTimeout(unsigned long seconds) {
  _connectTimeout = seconds * 1000;
}

void AsyncWiFiManager::setTryConnectDuringConfigPortal(boolean v) {
  _tryConnectDuringConfigPortal = v;
}

void AsyncWiFiManager::setDebugOutput(boolean debug) {
  _debug = debug;
}

void AsyncWiFiManager::setAPStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn) {
  _ap_static_ip = ip;
  _ap_static_gw = gw;
  _ap_static_sn = sn;
}

void AsyncWiFiManager::setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn, IPAddress dns1, IPAddress dns2) {
  _sta_static_ip = ip;
  _sta_static_gw = gw;
  _sta_static_sn = sn;
  _sta_static_dns1 = dns1;
  _sta_static_dns2 = dns2;
}

void AsyncWiFiManager::setMinimumSignalQuality(int quality) {
  _minimumQuality = quality;
}

void AsyncWiFiManager::setBreakAfterConfig(boolean shouldBreak) {
  _shouldBreakAfterConfig = shouldBreak;
}

/** Handle root or redirect to captive portal */
void AsyncWiFiManager::handleRoot(AsyncWebServerRequest *request) {
  // AJS - maybe we should set a scan when we get to the root???
  // and only scan on demand? timer + on demand? plus a link to make it happen?
  shouldscan=true;
  scannow= -1 ;
  DEBUG_WM(F("Handle root"));
  if (captivePortal(request)) { // If captive portal redirect instead of displaying the page.
    return;
  }

  String page = FPSTR(WFM_HTTP_HEAD);
  page.replace("{v}", "Options");
  page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_STYLE);
  page += _customHeadElement;
  page += FPSTR(HTTP_HEAD_END);
  page += "<h1>";
  page += _apName;
  page += "</h1>";
  page += F("<h3>AsyncWiFiManager</h3>");
  page += FPSTR(HTTP_PORTAL_OPTIONS);
  page += _customOptionsElement;
  page += FPSTR(HTTP_END);

  request->send(200, "text/html", page);

}

/** Wifi config page handler */
void AsyncWiFiManager::handleWifi(AsyncWebServerRequest *request,boolean scan) {
  shouldscan=true;
  scannow= -1 ;

  DEBUG_WM(F("Handle wifi"));

  String page = FPSTR(WFM_HTTP_HEAD);
  page.replace("{v}", "Config ESP");
  page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_STYLE);
  page += _customHeadElement;
  page += FPSTR(HTTP_HEAD_END);

  if (scan) {
    wifiSSIDscan=false;



    DEBUG_WM(F("Scan done"));
    if (wifiSSIDCount==0) {
      DEBUG_WM(F("No networks found"));
      page += F("No networks found. Refresh to scan again.");
    } else {


      //display networks in page
      String pager = networkListAsString();

      page += pager;
      page += "<br/>";
    }

  }
  wifiSSIDscan=true;

  page += FPSTR(HTTP_FORM_START);
  char parLength[2];
  // add the extra parameters to the form
  for (int i = 0; i < _paramsCount; i++) {
    if (_params[i] == NULL) {
      break;
    }

    String pitem = FPSTR(HTTP_FORM_PARAM);
    if (_params[i]->getID() != NULL) {
      pitem.replace("{i}", _params[i]->getID());
      pitem.replace("{n}", _params[i]->getID());
      pitem.replace("{p}", _params[i]->getPlaceholder());
      snprintf(parLength, 2, "%d", _params[i]->getValueLength());
      pitem.replace("{l}", parLength);
      pitem.replace("{v}", _params[i]->getValue());
      pitem.replace("{c}", _params[i]->getCustomHTML());
    } else {
      pitem = _params[i]->getCustomHTML();
    }

    page += pitem;
  }
  if (_params[0] != NULL) {
    page += "<br/>";
  }

  if (_sta_static_ip) {

    String item = FPSTR(HTTP_FORM_PARAM);
    item.replace("{i}", "ip");
    item.replace("{n}", "ip");
    item.replace("{p}", "Static IP");
    item.replace("{l}", "15");
    item.replace("{v}", _sta_static_ip.toString());

    page += item;

    item = FPSTR(HTTP_FORM_PARAM);
    item.replace("{i}", "gw");
    item.replace("{n}", "gw");
    item.replace("{p}", "Static Gateway");
    item.replace("{l}", "15");
    item.replace("{v}", _sta_static_gw.toString());

    page += item;

    item = FPSTR(HTTP_FORM_PARAM);
    item.replace("{i}", "sn");
    item.replace("{n}", "sn");
    item.replace("{p}", "Subnet");
    item.replace("{l}", "15");
    item.replace("{v}", _sta_static_sn.toString());

    page += item;

    item = FPSTR(HTTP_FORM_PARAM);
    item.replace("{i}", "dns1");
    item.replace("{n}", "dns1");
    item.replace("{p}", "DNS1");
    item.replace("{l}", "15");
    item.replace("{v}", _sta_static_dns1.toString());

    page += item;

    item = FPSTR(HTTP_FORM_PARAM);
    item.replace("{i}", "dns2");
    item.replace("{n}", "dns2");
    item.replace("{p}", "DNS2");
    item.replace("{l}", "15");
    item.replace("{v}", _sta_static_dns2.toString());

    page += item;

    page += "<br/>";
  }

  page += FPSTR(HTTP_FORM_END);
  page += FPSTR(HTTP_SCAN_LINK);

  page += FPSTR(HTTP_END);

  request->send(200, "text/html", page);


  DEBUG_WM(F("Sent config page"));
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void AsyncWiFiManager::handleWifiSave(AsyncWebServerRequest *request) {
  DEBUG_WM(F("WiFi save"));

  //SAVE/connect here
  needInfo = true;
  _ssid = request->arg("s").c_str();
  _pass = request->arg("p").c_str();

  //parameters
  for (int i = 0; i < _paramsCount; i++) {
    if (_params[i] == NULL) {
      break;
    }
    //read parameter
    String value = request->arg(_params[i]->getID()).c_str();
    //store it in array
    value.toCharArray(_params[i]->_value, _params[i]->_length);
    DEBUG_WM(F("Parameter"));
    DEBUG_WM(_params[i]->getID());
    DEBUG_WM(value);
  }

  if (request->hasArg("ip")) {
    DEBUG_WM(F("static ip"));
    DEBUG_WM(request->arg("ip"));
    //_sta_static_ip.fromString(request->arg("ip"));
    String ip = request->arg("ip");
    optionalIPFromString(&_sta_static_ip, ip.c_str());
  }
  if (request->hasArg("gw")) {
    DEBUG_WM(F("static gateway"));
    DEBUG_WM(request->arg("gw"));
    String gw = request->arg("gw");
    optionalIPFromString(&_sta_static_gw, gw.c_str());
  }
  if (request->hasArg("sn")) {
    DEBUG_WM(F("static netmask"));
    DEBUG_WM(request->arg("sn"));
    String sn = request->arg("sn");
    optionalIPFromString(&_sta_static_sn, sn.c_str());
  }
  if (request->hasArg("dns1")) {
    DEBUG_WM(F("static DNS 1"));
    DEBUG_WM(request->arg("dns1"));
    String dns1 = request->arg("dns1");
    optionalIPFromString(&_sta_static_dns1, dns1.c_str());
  }
  if (request->hasArg("dns2")) {
    DEBUG_WM(F("static DNS 2"));
    DEBUG_WM(request->arg("dns2"));
    String dns2 = request->arg("dns2");
    optionalIPFromString(&_sta_static_dns2, dns2.c_str());
  }

  String page = FPSTR(WFM_HTTP_HEAD);
  page.replace("{v}", "Credentials Saved");
  page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_STYLE);
  page += _customHeadElement;
  page += F("<meta http-equiv=\"refresh\" content=\"5; url=/i\">");
  page += FPSTR(HTTP_HEAD_END);
  page += FPSTR(HTTP_SAVED);
  page += FPSTR(HTTP_END);

  request->send(200, "text/html", page);

  DEBUG_WM(F("Sent wifi save page"));

  connect = true; //signal ready to connect/reset
}

/** Handle the info page */
String AsyncWiFiManager::infoAsString()
{
  String page;
  page += F("<dt>Chip ID</dt><dd>");
#if defined(ESP8266)
  page += ESP.getChipId();
#else
  page += getESP32ChipID();
#endif
  page += F("</dd>");
  page += F("<dt>Flash Chip ID</dt><dd>");
#if defined(ESP8266)
  page += ESP.getFlashChipId();
#else
  page += F("N/A for ESP32");
#endif
  page += F("</dd>");
  page += F("<dt>IDE Flash Size</dt><dd>");
  page += ESP.getFlashChipSize();
  page += F(" bytes</dd>");
  page += F("<dt>Real Flash Size</dt><dd>");
#if defined(ESP8266)
  page += ESP.getFlashChipRealSize();
#else
  page += F("N/A for ESP32");
#endif
  page += F(" bytes</dd>");
  page += F("<dt>Soft AP IP</dt><dd>");
  page += WiFi.softAPIP().toString();
  page += F("</dd>");
  page += F("<dt>Soft AP MAC</dt><dd>");
  page += WiFi.softAPmacAddress();
  page += F("</dd>");
  page += F("<dt>Station SSID</dt><dd>");
  page += WiFi.SSID();
  page += F("</dd>");
  page += F("<dt>Station IP</dt><dd>");
  page += WiFi.localIP().toString();
  page += F("</dd>");
  page += F("<dt>Station MAC</dt><dd>");
  page += WiFi.macAddress();
  page += F("</dd>");
  page += F("</dl>");
  return page;
}

void AsyncWiFiManager::handleInfo(AsyncWebServerRequest *request) {
  DEBUG_WM(F("Info"));

  String page = FPSTR(WFM_HTTP_HEAD);
  page.replace("{v}", "Info");
  page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_STYLE);
  page += _customHeadElement;
  if (connect==true)
  page += F("<meta http-equiv=\"refresh\" content=\"5; url=/i\">");
  page += FPSTR(HTTP_HEAD_END);
  page += F("<dl>");
  if (connect==true)
  {
    page += F("<dt>Trying to connect</dt><dd>");
    page += wifiStatus;
    page += F("</dd>");
  }

  page +=pager;
  page += FPSTR(HTTP_END);

  request->send(200, "text/html", page);

  DEBUG_WM(F("Sent info page"));
}

/** Handle the reset page */
void AsyncWiFiManager::handleReset(AsyncWebServerRequest *request) {
  DEBUG_WM(F("Reset"));

  String page = FPSTR(WFM_HTTP_HEAD);
  page.replace("{v}", "Info");
  page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_STYLE);
  page += _customHeadElement;
  page += FPSTR(HTTP_HEAD_END);
  page += F("Module will reset in a few seconds.");
  page += FPSTR(HTTP_END);
  request->send(200, "text/html", page);

  DEBUG_WM(F("Sent reset page"));
  delay(5000);
  #if defined(ESP8266)
  ESP.reset();
  #else
  ESP.restart();
  #endif
  delay(2000);
}



//removed as mentioned here https://github.com/tzapu/AsyncWiFiManager/issues/114
/*void AsyncWiFiManager::handle204(AsyncWebServerRequest *request) {
DEBUG_WM(F("204 No Response"));
request->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
request->sendHeader("Pragma", "no-cache");
request->sendHeader("Expires", "-1");
request->send ( 204, "text/plain", "");

}*/

void AsyncWiFiManager::handleNotFound(AsyncWebServerRequest *request) {
  DEBUG_WM(F("Handle not found"));
  if (captivePortal(request)) { // If captive portal redirect instead of displaying the error page.
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += ( request->method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += request->args();
  message += "\n";

  for ( uint8_t i = 0; i < request->args(); i++ ) {
    message += " " + request->argName ( i ) + ": " + request->arg ( i ) + "\n";
  }
  AsyncWebServerResponse *response = request->beginResponse(404,"text/plain",message);
  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "-1");
  request->send (response );
}


/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean AsyncWiFiManager::captivePortal(AsyncWebServerRequest *request) {
  if (!isIp(request->host()) ) {
    DEBUG_WM(F("Request redirected to captive portal"));
    AsyncWebServerResponse *response = request->beginResponse(302,"text/plain","");
    response->addHeader("Location", String("http://") + toStringIp(request->client()->localIP()));
    request->send ( response);
    return true;
  }
  return false;
}

//start up config portal callback
void AsyncWiFiManager::setAPCallback( void (*func)(AsyncWiFiManager* myAsyncWiFiManager) ) {
  _apcallback = func;
}

//start up save config callback
void AsyncWiFiManager::setSaveConfigCallback( void (*func)(void) ) {
  _savecallback = func;
}

//sets a custom element to add to head, like a new style tag
void AsyncWiFiManager::setCustomHeadElement(const char* element) {
  _customHeadElement = element;
}

//sets a custom element to add to options page
void AsyncWiFiManager::setCustomOptionsElement(const char* element) {
  _customOptionsElement = element;
}

//if this is true, remove duplicated Access Points - defaut true
void AsyncWiFiManager::setRemoveDuplicateAPs(boolean removeDuplicates) {
  _removeDuplicateAPs = removeDuplicates;
}



template <typename Generic>
void AsyncWiFiManager::DEBUG_WM(Generic text) {
  if (_debug) {
    Serial.print("*WM: ");
    Serial.println(text);
  }
}

int AsyncWiFiManager::getRSSIasQuality(int RSSI) {
  int quality = 0;

  if (RSSI <= -100) {
    quality = 0;
  } else if (RSSI >= -50) {
    quality = 100;
  } else {
    quality = 2 * (RSSI + 100);
  }
  return quality;
}

/** Is this an IP? */
boolean AsyncWiFiManager::isIp(String str) {
  for (int i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String AsyncWiFiManager::toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}
