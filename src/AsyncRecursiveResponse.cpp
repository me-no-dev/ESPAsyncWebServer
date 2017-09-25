// *************************************************************************************
//	AsyncRecursiveResponse.cpp
//
//		AsyncRecursiveResponse implementation.
//		
//																																		Ryan Boghean
//																																		September 2017
//
// *************************************************************************************

# pragma once

# include <cstdint>
# include <functional>

# include <pgmspace.h>
# include <WString.h>

# include "PGMHelper.h"
# include "StringSource.h"
# include "StringWrapper.h"
# include "ESPAsyncWebServer.h"
# include "AsyncRecursiveResponse.h"

//using namespace ESPAsync;


// --------------------------------------------------------------------------------------------------------------------------
//	AsyncRecursiveReponse
// --------------------------------------------------------------------------------------------------------------------------

// ----- standard member functions -----
AsyncRecursiveReponse::AsyncRecursiveReponse(AsyncWebServerRequest* request_, RecursiveResponseCallback call_back_, StringWrapper src, char sentinel_) :
	call_back(std::move(call_back_)) {
	request = request_;
	state = RESPONSE_SETUP;
	stack = nullptr;
	sentinel = sentinel_;
	PushStack(std::move(src));
	}

AsyncRecursiveReponse::AsyncRecursiveReponse(AsyncRecursiveReponse&& rr) :
	call_back(std::move(rr.call_back)) {

	request = rr.request; rr.request = nullptr;
	state = rr.state; rr.state = RESPONSE_FAILED;
	stack = rr.stack; rr.stack = nullptr;
	sentinel = rr.sentinel; rr.sentinel = 0;
	}

AsyncRecursiveReponse& AsyncRecursiveReponse::operator=(AsyncRecursiveReponse&& rr) {
	if (this == &rr) return *this;

	request = rr.request; rr.request = nullptr;
	state = rr.state; rr.state = RESPONSE_FAILED;
	call_back = std::move(rr.call_back);
	stack = rr.stack; rr.stack = nullptr;
	sentinel = rr.sentinel; rr.sentinel = 0;
		
	return *this;
	}

AsyncRecursiveReponse::~AsyncRecursiveReponse() {
	ClearStack();
	}


// ----- static helpers -----
const char* AsyncRecursiveReponse::HTTPCodeString(int code) {
	switch (code) {
		case 100: return "100 Continue\r\n";
		case 101: return "101 Switching Protocols\r\n";
		case 200: return "200 OK\r\n";
		case 201: return "201 Created\r\n";
		case 202: return "202 Accepted\r\n";
		case 203: return "203 Non-Authoritative Information\r\n";
		case 204: return "204 No Content\r\n";
		case 205: return "205 Reset Content\r\n";
		case 206: return "206 Partial Content\r\n";
		case 300: return "300 Multiple Choices\r\n";
		case 301: return "301 Moved Permanently\r\n";
		case 302: return "302 Found\r\n";
		case 303: return "303 See Other\r\n";
		case 304: return "304 Not Modified\r\n";
		case 305: return "305 Use Proxy\r\n";
		case 307: return "307 Temporary Redirect\r\n";
		case 400: return "400 Bad Request\r\n";
		case 401: return "401 Unauthorized\r\n";
		case 402: return "402 Payment Required\r\n";
		case 403: return "403 Forbidden\r\n";
		case 404: return "404 Not Found\r\n";
		case 405: return "405 Method Not Allowed\r\n";
		case 406: return "406 Not Acceptable\r\n";
		case 407: return "407 Proxy Authentication Required\r\n";
		case 408: return "408 Request Time-out\r\n";
		case 409: return "409 Conflict\r\n";
		case 410: return "410 Gone\r\n";
		case 411: return "411 Length Required\r\n";
		case 412: return "412 Precondition Failed\r\n";
		case 413: return "413 Request Entity Too Large\r\n";
		case 414: return "414 Request-URI Too Large\r\n";
		case 415: return "415 Unsupported Media Type\r\n";
		case 416: return "416 Requested range not satisfiable\r\n";
		case 417: return "417 Expectation Failed\r\n";
		case 500: return "500 Internal Server Error\r\n";
		case 501: return "501 Not Implemented\r\n";
		case 502: return "502 Bad Gateway\r\n";
		case 503: return "503 Service Unavailable\r\n";
		case 504: return "504 Gateway Time-out\r\n";
		case 505: return "505 HTTP Version not supported\r\n";
		default: return "";
		}
	}


// ----- stack helpers -----
void AsyncRecursiveReponse::PushStack(StringWrapper&& str) {

	// get node (reuse old if possible, otherwise create new)
	RecursiveReponseStack* node;
	if (stack && stack->next) {
		node = stack->next;
		}
	else {
		node = new RecursiveReponseStack();
		node->next = nullptr;
		node->prev = stack;
		if (stack) stack->next = node;
		}

	// assign data
	node->str = std::move(str);
	node->begin = node->str.Data();

	// adjust stack
	stack = node;
	}

void AsyncRecursiveReponse::PopStack() {
	if (stack == nullptr) return;

	// clear data
	stack->str.Clear();
	stack->begin = nullptr;

	// 'pop' from stack
	stack = stack->prev;
	}

void AsyncRecursiveReponse::ClearStack() {
	if (stack == nullptr) return;

	// move to top of stack
	while (stack->next) stack = stack->next;

	// delete stack
	do {
		RecursiveReponseStack* prev_node = stack->prev;
		delete stack;
		stack = prev_node;
		} while (stack);
	}


// ----- internal helpers -----
AsyncRecursiveReponse::SubStringCode AsyncRecursiveReponse::ReadSubString(const char*& ptr) const {

	SubStringCode code = SubStringCode::exceeded_len;
	char sentinel_ = sentinel;
	ptr = IteratePGM(ptr,
		[&code, sentinel_](char c) -> bool { 
			if (c == 0) { code = SubStringCode::end_of_string; return false; }
			if (c == sentinel_) { code = SubStringCode::found_sentinel; return false; }
			return true; 
			}
		);

	return code;
	}

AsyncRecursiveReponse::SubStringCode AsyncRecursiveReponse::ReadSubString(const char*& ptr, size_t n) const {

	SubStringCode code = SubStringCode::exceeded_len;
	char sentinel_ = sentinel;
	ptr = IteratePGM(ptr, n,
		[&code, &n, sentinel_](char c) -> bool {
			if (c == 0) { code = SubStringCode::end_of_string; return false; }
			if (c == sentinel_) { code = SubStringCode::found_sentinel; return false; }
			return true; 
			}
		);

	return code;
	}

bool AsyncRecursiveReponse::WriteHeader(const char* str, StringWrapperType type) {
	// header content is small, and so is copied directly into the lwip buffer
	// docs say it is the most efficient method for small strings
	// also means I don't need to track lifetime of header string data

	// add() directly calls tcp_write()
	// it returns the number of bytes transmitted, but it can only return 'n' or 0, nothing in between
	// so the return choice is kinda strange, IMHO an error code or boolean would be preferable...

	// headers are pretty small, most of the time they will be sent (cached) successfully
	// and in the rare case they don't we send the packet and immediately try again
	// we don't have to wait for the ack, more data will be automatically allocated by lwip from its
	// internal ~16k pool
	// if we run out of the ~16k pool simply from headers... we've done something wrong
	// there seems to me to be no point in simply storeing headers in dynamic memory (something we
	// really need to conserve on an ESP8266) when there is already dynamic memory allocated for us to use
	// ie. the lwip pool

	size_t str_len;

	// progmem handler
	if (type == StringWrapperType::st_progmem) {
		str_len = strlen_P(str);
# ifndef ESPASYNC_PROGMEM_SAFE
		// progmem is not safe to use, so we copy to temporary buffer
		char* str_cpy = static_cast<char*>(alloca(str_len));		// can replace with malloc but alloca is much faster
		//if (str_cpy == nullptr) return false;									// not sure if alloca is checked, or even if it matters if it was
		memcpy_P(str_cpy, str, str_len);
		str = str_cpy;
# endif
		}
	else {
		str_len = strlen(str);
		}

	// send data
	if (request->client()->add(str, str_len, TCP_WRITE_FLAG_COPY) == str_len) return true;		// try first time
	if (!request->client()->send()) return false;																			// attempt send
	return request->client()->add(str, str_len, TCP_WRITE_FLAG_COPY) == str_len;					// try second time (if this fails we can't recover)
	}

bool AsyncRecursiveReponse::WriteBody(const char* str, size_t n, StringWrapperType type) {

	uint8_t flags = 0;			// use PSH flag??

# ifndef ESPASYNC_PROGMEM_SAFE
	char* str_cpy = nullptr;
	if (type == StringWrapperType::st_progmem) {
		str_cpy = static_cast<char*>(malloc(n));			// large strings could blow the stack
		if (str_cpy == nullptr) return false;
		memcpy_P(str_cpy, str, n);
		str = str_cpy;
		flags |= TCP_WRITE_FLAG_COPY;
		}
# endif

	// the pbuf structure in lwip is 20 bytes on the ESP8266
	// which means its better to copy for strings smaller than 20 bytes than to add another entry to the lwip linked list
	// progmem strings, even with ESPASYNC_PROGMEM_SAFE, need to be copied in (I think), since the copy into the buffer is safe, but accesing the buffer from whatever internal data the ESP8266 has is probably not
	// also dynamic strings may be delete'd before the next ack, so its better to copy them
	// bottom line is, when in doubt let lwip handle memory management
	// it already has memory allocated for that purpose, and is easy to customize through #define policies
	if (n <= 20 || type == StringWrapperType::st_progmem || type == StringWrapperType::st_dynamic_new || type == StringWrapperType::st_dynamic_malloc) flags |= TCP_WRITE_FLAG_COPY;
	bool success = request->client()->add(str, n, flags) == n;

# ifndef ESPASYNC_PROGMEM_SAFE
	if (str_cpy) free(str_cpy);
# endif

	return success;
	}

bool AsyncRecursiveReponse::Send() {
	return request->client()->send();
	}

void AsyncRecursiveReponse::Close(bool now) {
	request->client()->close(now);			// even though this is called 'now' it seems on success we set to true, otherwise leave false?  I don't really understand why...
	}

size_t AsyncRecursiveReponse::Space() {
	return request->client()->space();
	}

void AsyncRecursiveReponse::Fail() {
	state = RESPONSE_FAILED;
	request->client()->close(false);
	}


// ----- AsyncWebServerResponse interface -----
void AsyncRecursiveReponse::setCode(int code) {

	if (state != RESPONSE_SETUP) { Fail(); return; }

	// write inital HTTP header
	// if any WriteHeader()'s fail, the whole transmission fails
	// we've already attempted to recover and that has also failed
	// so just report the failure, there's nothing more we can do
	int http_ver = 1;
	bool success = true;
	success &= WriteHeader("HTTP/1.1 ", StringWrapperType::st_literal);
	success &= WriteHeader(HTTPCodeString(code), StringWrapperType::st_literal);
	if (!success) { Fail(); return; }

	// ready to set more headers
	state = RESPONSE_HEADERS;
	}

void AsyncRecursiveReponse::AsyncRecursiveReponse::setContentLength(size_t len) {
	// intentionally do nothing here
	}

void AsyncRecursiveReponse::setContentType(const String& type) {

	if (state != RESPONSE_HEADERS) { Fail(); return; }

	bool success = true;
	success &= WriteHeader("Content-Type: ", StringWrapperType::st_literal);
	success &= WriteHeader(type.c_str(), StringWrapperType::st_dynamic_new);				// doesn't matter here as long as its not progmem
	success &= WriteHeader("\r\n", StringWrapperType::st_literal);
	if (!success) Fail();
	}

void AsyncRecursiveReponse::setContentType(const StringWrapper& type) {

	if (state != RESPONSE_HEADERS) { Fail(); return; }

	bool success = true;
	success &= WriteHeader("Content-Type: ", StringWrapperType::st_literal);
	success &= WriteHeader(type.Data(), type.Type());
	success &= WriteHeader("\r\n", StringWrapperType::st_literal);
	if (!success) Fail();
	}

void AsyncRecursiveReponse::addHeader(const String& name, const String& value) {

	if (state != RESPONSE_HEADERS) { Fail(); return; }

	bool success = true;
	success &= WriteHeader(name.c_str(), StringWrapperType::st_dynamic_new);
	success &= WriteHeader(": ", StringWrapperType::st_literal);
	success &= WriteHeader(value.c_str(), StringWrapperType::st_dynamic_new);
	success &= WriteHeader("\r\n", StringWrapperType::st_literal);
	if (!success) Fail();
	}

void AsyncRecursiveReponse::addHeader(const StringWrapper& name, const StringWrapper& value) {

	if (state != RESPONSE_HEADERS) { Fail(); return; }

	bool success = true;
	success &= WriteHeader(name.Data(), name.Type());
	success &= WriteHeader(": ", StringWrapperType::st_literal);
	success &= WriteHeader(value.Data(), value.Type());
	success &= WriteHeader("\r\n", StringWrapperType::st_literal);
	if (!success) Fail();
	}

String AsyncRecursiveReponse::_assembleHead(uint8_t version) {
	return String();			// do nothing
	}

bool AsyncRecursiveReponse::_started() const { 
	return state > RESPONSE_SETUP;
	}

bool AsyncRecursiveReponse::_finished() const { 
	return state > RESPONSE_WAIT_ACK;
	}

bool AsyncRecursiveReponse::_failed() const { 
	return state == RESPONSE_FAILED;
	}

bool AsyncRecursiveReponse::_sourceValid() const { 
	return true; 
	}

void AsyncRecursiveReponse::_respond(AsyncWebServerRequest* request) {

	if (state != RESPONSE_HEADERS) { Fail(); return; }

	// finish header with a final newline
	if (request->client()->add("\r\n", 2, TCP_WRITE_FLAG_COPY) == 0) { Fail(); return; }
	if (!Send()) { Fail(); return; }

	// ready for ack/content
	state = RESPONSE_WAIT_ACK;
	}

size_t AsyncRecursiveReponse::_ack(AsyncWebServerRequest* request, size_t n, uint32_t time) {
	
	if (state != RESPONSE_WAIT_ACK) { Fail(); return 0; }

	//n = Min(n, Space());			// not clear if this is necessary
	n = Space();							// not sure exactly what 'n' is for....
	if (n == 0) return 0;
	size_t bytes_sent = 0;

	// process string
	while (true) {

		// load stack variables
		const char*& begin = stack->begin;
		StringWrapperType str_type = stack->str.Type();

		// read and send next substring
		const char* end = begin;
		SubStringCode ssc = ReadSubString(end, n);
		size_t len = static_cast<size_t>(end - begin);
		if (len && !WriteBody(begin, len, str_type)) {
			if (bytes_sent == 0) Fail();		// only set failed if no bytes were sent, in that case its unlikely to recover
			return bytes_sent;
			}
		bytes_sent += len;
		n -= len;

		// end of string
		if (ssc == SubStringCode::end_of_string) {
			PopStack();
			if (stack == nullptr) {
				if (!Send()) Fail();
				else {
					state = RESPONSE_END;		// successful parse
					Close(true);
					}
				return bytes_sent;
				}
			}
		else if(ssc == SubStringCode::found_sentinel) {
			begin = end + 1;
			end = begin;
			if (ReadSubString(end) == SubStringCode::found_sentinel) {
				PushStack(call_back(StringSource(begin, end)));		// retrieve string associated with template
				begin = end + 1;														// update old begin to point passed the end of the template id
				}
			else {
				// template was not finished properly (hit eos before the next sentinel)
				Fail();
				return bytes_sent;
				}
			}
		else {
			// we didn't hit end of string, or a sentinel, so we must have simply hit the send limit for the packet
			// send what we have and continue transfer on next ack
			begin = end;
			if (!Send()) Fail();
			return bytes_sent;
			}
		}
	}
