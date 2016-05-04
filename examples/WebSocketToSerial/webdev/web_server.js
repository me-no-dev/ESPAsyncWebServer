// ======================================================================
//                      ESP8266 WEB Server Simulator
// ======================================================================
// This file is not part of web server, it's just used as ESP8266 
// Simulator to check HTLM / JQuery and all web stuff without needing 
// to flash ESP8266 target, you'll need nodejs to run it
// Please install dependencies with
// npm install mime httpdispatcher websocket
// after all is installed just start by typing on command line
// node web_server.js
// once all is fine, just run the script create_spiffs.js to publish
// files to data folder, then you can upload data tiles with Arduino IDE
// ======================================================================

// Dependencies
var http = require('http');
var fs = require('fs');
var path = require('path');
var url = require('url');
var mime = require('mime');
var util = require('util');
var os = require('os');
var dispatcher = require('httpdispatcher');
var ws = require('websocket').server;

//Lets use our dispatcher
function handleRequest(req, res) {
  try {
    console.log(req.url);
    dispatcher.dispatch(req, res);
  } 
  catch(err) {
    console.log(err);
  }
}

function humanSize(bytes) {
	var units =  ['kB','MB','GB','TB','PB','EB','ZB','YB']
	var thresh = 1024;
	if(Math.abs(bytes) < thresh) 
		return bytes + ' B';

	var u = -1;
	do {
		bytes /= thresh;
		++u;
	} 
	while(Math.abs(bytes) >= thresh && u < units.length - 1);
	return bytes.toFixed(1)+' '+units[u];
}

dispatcher.onGet("/heap", function(req, res) {
      res.writeHead(200, {"Content-Type": "text/html"});
      res.end(humanSize(os.freemem()));
});    

dispatcher.onError(function(req, res) {
  var uri = url.parse(req.url).pathname;
	var filePath = '.' + uri;
	var extname = path.extname(filePath);

	if (filePath == './')
  	filePath = './index.htm';

	var contentType = mime.lookup(filePath);

	console.log('filepath='+filePath);

	fs.readFile(filePath, function(error, content) {
    if (error) {
      if(error.code == 'ENOENT'){
        fs.readFile('./404.html', function(error, content) {
          res.writeHead(200, { 'Content-Type': contentType });
          res.end(content, 'utf-8');
					console.log("ENOENT "+filePath+ ' => '+contentType);
        });
      }
      else {
        res.writeHead(500);
        res.end('Sorry, check with the site admin for error: '+error.code+' ..\n');
        res.end(); 
				console.log("Error "+filePath+ ' => '+contentType);
      }
    }
    else {
      res.writeHead(200, { 'Content-Type': contentType });
      res.end(content, 'utf-8');
			console.log("Sent "+filePath+ ' => '+contentType);
    }
  });
});

// Create HTTP server + WebSocket
var server = http.createServer(handleRequest);
var wsSrv = new ws({ httpServer: server });

wsSrv.on('request', function(request) {
	var connection = request.accept('', request.origin);
	console.log("+++ Websocket client connected!");

	connection.on('message', function(message) {
		//console.log(connection, 'ws message ' + util.inspect(message, false, null));

		if (message.type === 'utf8') {
			var msg = message.utf8Data.split(':');
			var value = msg[1];
			msg = msg[0]
			console.log(' msg="' + msg + '"  value="'+value+'"');

			if (msg==='ping') {
				connection.sendUTF('pong');
			} else {
				connection.sendUTF("I've received your '"+msg+"' message")
			}
		}
		else if (message.type === 'binary') {
			console.log('Received Binary Message of ' + message.binaryData.length + ' bytes');
			connection.sendBytes(message.binaryData);
		}
	});

	connection.on('close', function(reasonCode, description) {
		console.log((new Date()) + ' Peer ' + connection.remoteAddress + ' disconnected.');
	});
});

//Lets start our server
server.listen(8080, function() {
  //Callback triggered when server is successfully listening. Hurray!
  console.log("Server listening on: http://localhost:%s", 8080);
});
