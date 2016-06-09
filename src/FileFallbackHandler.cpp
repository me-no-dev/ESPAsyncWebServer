


#include "FileFallbackHandler.h"
#include <ESPAsyncWebServer.h>

bool FileFallbackHandler::canHandle(AsyncWebServerRequest *request)
{
	if (request->method() != HTTP_GET) {
		return false;
	}
	if ((_isFile && request->url() != _uri) ) {
		return false;
	}
	//  if the root of the request matches the _uri then it checks to see if there is a file it can handle.
	if (request->url().startsWith(_uri)) {
		String path = _getPath(request);
		if (_fs.exists(path) || _fs.exists(path + ".gz")) {
			DEBUGF("[FileFallbackHandler::canHandle] TRUE\n");
			return true;
		}
	}

	return false;
}

String FileFallbackHandler::_getPath(AsyncWebServerRequest *request)
{

	String path = request->url();
	DEBUGF("[FileFallbackHandler::_getPath]\n");
	DEBUGF("  [stored] _uri = %s, _path = %s\n" , _uri.c_str(), _path.c_str() ) ;
	DEBUGF("  [request] url = %s\n", request->url().c_str() );

	if (!_isFile) {
		DEBUGF("   _isFile = false\n");
		String baserequestUrl = request->url().substring(_uri.length());  // this is the request - stored _uri...  /espman/
		DEBUGF("  baserequestUrl = %s\n", baserequestUrl.c_str());

		if (!baserequestUrl.length()) {
			baserequestUrl += "/";
		}

		path = _path + baserequestUrl;
		DEBUGF("  path = path + baserequestUrl, path = %s\n", path.c_str());

		if (path.endsWith("/")) {
			DEBUGF("  3 path ends with / : path = index.htm \n");
			path += "index.htm";
		}
	} else {
		path = _path;
	}

	DEBUGF(" final path = %s\n", path.c_str());
	DEBUGF("[FileFallbackHandler::_getPath] END\n\n");

	return path;
}


void FileFallbackHandler::handleRequest(AsyncWebServerRequest *request)
{

	String path = _getPath(request);


	if (  request->client()->localIP() == WiFi.localIP() ) {

		AsyncWebServerResponse *response = request->beginResponse(302); //Sends 404 File Not Found
		response->addHeader("Location", _forwardUri );
		if (_cache_header.length() != 0) {
			response->addHeader("Cache-Control", _cache_header);
		}
		request->send(response);

	} else if (_fs.exists(path) || _fs.exists(path + ".gz")) {
		AsyncWebServerResponse * response = request->beginResponse(_fs, path);
		if (_cache_header.length() != 0) {
			response->addHeader("Cache-Control", _cache_header);
		}
		request->send(response);
	} else {
		request->send(404);
	}

	path = String();

}