// *************************************************************************************
//	AsyncRecursiveResponse.h
//
//		AsyncRecursiveResponse header class.
//		Effecient web response that uses recursion to allow large, dynamic, web content with minimal memory.
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


// --------------------------------------------------------------------------------------------------------------------------
//	ESPAsync namespace
// --------------------------------------------------------------------------------------------------------------------------

// ESPAsync should have a namespace...
//namespace ESPAsync {

// prior to including lwip, we need to define MEMCPY to memcpy_P if we want to support PROGMEM strings
// at the same time we should also then define ESPASYNC_PROGMEM_SAFE
// I'm not sure exactly how it should be properly exposed, and would require modifying source files
// that I didn't write, so I'll just leave this comment here for now...
//# define MEMCPY memcpy_P
//# define ESPASYNC_PROGMEM_SAFE

// even without this definition PROGMEMs can be used, but they are copied to dynamic memory, which
// could be an issue for large strings


// --------------------------------------------------------------------------------------------------------------------------
//	forward declarations
// --------------------------------------------------------------------------------------------------------------------------

class RecursiveNode;
class AsyncRecursiveResponse;

typedef std::function<RecursiveNode(StringSource)> RecursiveResponseCallback;


// --------------------------------------------------------------------------------------------------------------------------
//	RecursiveNode
//		- used for passing data from the callback to AsyncRecursiveResponse
//		- wraps a string and a callback function to call when templates are found in the string
//
//	implemented in: AsycnRecursiveResponse.cpp
// --------------------------------------------------------------------------------------------------------------------------

class RecursiveNode {
	friend class AsyncRecursiveReponse;

	private:
		RecursiveResponseCallback callback;
		StringWrapper str;
		const char* begin;									// offset into data string (ie. current point of parse), called begin because its the begining of the next substring
		RecursiveNode* prev;							// previous node

	public:
		RecursiveNode(RecursiveResponseCallback, StringWrapper);
		template<class... TArgs> RecursiveNode(RecursiveResponseCallback, TArgs&&...);

		RecursiveNode(const RecursiveNode&) = delete;
		RecursiveNode& operator=(const RecursiveNode&) = delete;
		RecursiveNode(RecursiveNode&&);
		RecursiveNode& operator=(RecursiveNode&&);

		~RecursiveNode();
	};


// ----- template code -----
template<class... TArgs> RecursiveNode::RecursiveNode(RecursiveResponseCallback callback_, TArgs&&... args) :
	callback(std::move(callback_)),
	str(std::forward<TArgs>(args)...) {
	begin = str.Data();
	prev = nullptr;
	}


// --------------------------------------------------------------------------------------------------------------------------
//	AsyncRecursiveReponse
//		- ??
//
//	implemented in: AsyncRecursive.cpp
// --------------------------------------------------------------------------------------------------------------------------

// ----- AsyncRecursiveReponse -----
class AsyncRecursiveReponse : public AsyncWebServerResponse {

	private:
		// internal constants
		enum class SubStringCode { end_of_string, found_sentinel, exceeded_len };
		static constexpr uint8_t TCP_WRITE_FLAG_COPY = 0x01;

		// internal data
		AsyncWebServerRequest* request;
	    WebResponseState state;
		RecursiveNode* stack;
		char sentinel;

		// static helpers
		static const char* HTTPCodeString(int code);

		// stack helpers
		void PushStack(RecursiveNode&&);
		void PopStack();
		void ClearStack();

		// internal helpers
		SubStringCode ReadSubString(const char*& ptr) const;
		SubStringCode ReadSubString(const char*& ptr, size_t n) const;
		bool WriteHeader(const char*, StringWrapperType);
		bool WriteBody(const char*, size_t n, StringWrapperType);
		bool Send();
		void Close(bool now);
		size_t Space();
		void Fail();

	public:
		AsyncRecursiveReponse(AsyncWebServerRequest*, RecursiveNode, char sentinel = '%');

		AsyncRecursiveReponse(const AsyncRecursiveReponse&) = delete;
		AsyncRecursiveReponse& operator=(const AsyncRecursiveReponse&) = delete;
		AsyncRecursiveReponse(AsyncRecursiveReponse&&);
		AsyncRecursiveReponse& operator=(AsyncRecursiveReponse&&);

		virtual ~AsyncRecursiveReponse();

		// AsyncWebServerResponse interface
		virtual void setCode(int code);
		virtual void setContentLength(size_t len);

		virtual void setContentType(const String& type);
		virtual void setContentType(const StringWrapper& type);

		virtual void addHeader(const String& name, const String& value);
		virtual void addHeader(const StringWrapper& name, const StringWrapper& value);

		virtual String _assembleHead(uint8_t version);
		virtual bool _started() const;
		virtual bool _finished() const;
		virtual bool _failed() const;
		virtual bool _sourceValid() const;
		virtual void _respond(AsyncWebServerRequest *request);
		virtual size_t _ack(AsyncWebServerRequest *request, size_t len, uint32_t time);
	};


// --------------------------------------------------------------------------------------------------------------------------
//	End 
// --------------------------------------------------------------------------------------------------------------------------

//}			// end of ESPAsync namespace