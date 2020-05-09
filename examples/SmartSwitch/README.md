![](1.PNG) ![](2.PNG) 
##
![](3.PNG) ![](4.PNG) 

## SmartSwitch
* Remote Temperature Control application with schedule (example car block heater or battery charger)
* Based on ESP_AsyncFSBrowser example with ACE editor 
* Wide browser compatibility, no extra server-side needed
* HTTP server and WebSocket, single port  
* Standalone, no JS dependencies for the browser from Internet (I hope), ace editor included
* Added ESPAsyncWiFiManager
* Real Time (NTP) w/ Time Zones
* Memorized settings to EEPROM
* Multiple clients can be connected at same time, they see each other' requests
* Base Authentication of the editor, static content, WS
* Or Cookie Authentication including WS part, need lib src changes taken from https://github.com/me-no-dev/ESPAsyncWebServer/pull/684
* Default credentials <b>smart:switch</b>
* Use latest ESP8266 ESP32 cores from GitHub

