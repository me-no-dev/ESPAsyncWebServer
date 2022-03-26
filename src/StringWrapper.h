// *************************************************************************************
//	StringWrapper.h
//
//		Simple string wrapper.
//		Allows returning/working with literals, progmem's, and dynamically allocated strings.
//		
//																																		Ryan Boghean
//																																		September 2017
//
// *************************************************************************************

# pragma once

# include <cstdint>
//# include <ostream>		//??

# include <pgmspace.h>
# include <WString.h>

# include "StringSource.h"


// --------------------------------------------------------------------------------------------------------------------------
//	ESPAsync namespace
// --------------------------------------------------------------------------------------------------------------------------

//namespace ESPAsync {


// --------------------------------------------------------------------------------------------------------------------------
//	StringWrapper
//		- simple string wrapper
//		- has implict conversions from String() and __FlashStringHelper
//		- since const char* can come in many flavours, passing a const char* requires an explicit type specification
//		- does not support copying, but is moveable (by design)
//		- will always contain a valid (if empty) string, Data() will never return nullptr;
//		- it would be nice to have a way to 'steal' the internals of a String object, but I can't think of any
//			non-hackish ways of doing that without modifying String source...
//
//	implemented in: StringWrapper.cpp
// --------------------------------------------------------------------------------------------------------------------------

// ----- StringWrapperType -----
enum class StringWrapperType : uint8_t {
	st_literal,
	st_progmem,
	st_dynamic_new,
	st_dynamic_malloc,
	};


// ----- StringWrapperPrintfTag -----
struct StringWrapperPrintfTag {};
constexpr StringWrapperPrintfTag sw_printf;


// ----- StringWrapperStreamTag -----
struct StringWrapperStreamTag {};
constexpr StringWrapperStreamTag sw_stream;


// ----- StringWrapper -----
class StringWrapper {

	private:
		static const char* empty_string;			// empty string used for when no data is available
		const char* data;									// raw string data
		StringWrapperType type;						// type of string stored in data

		// internal helpers
		void AssignString(const String&);
		void MoveString(String&&);

	public:
		StringWrapper();
		StringWrapper(const String&);
		StringWrapper(String&&);
		StringWrapper(const __FlashStringHelper*);
		StringWrapper(const char*, StringWrapperType);
		template<class... TArgs> StringWrapper(StringWrapperPrintfTag, const char*, TArgs&&...);		// creates a dynamic string using snprintf()
		//template<class... TArgs> StringWrapper(StringWrapperStreamTag, TArgs&&...);					// creates a dynamic string using ostream??

		StringWrapper(const StringWrapper&) = delete;
		StringWrapper& operator=(const StringWrapper&) = delete;
		StringWrapper(StringWrapper&&);
		StringWrapper& operator=(StringWrapper&&);

		StringWrapper& operator=(const String&);
		StringWrapper& operator=(String&&);
		StringWrapper& operator=(const __FlashStringHelper*);

		~StringWrapper();

		// helper functions
		void Clear();
		const char* Data() const;								// be careful here, may be a progmem string
		StringWrapperType Type() const;
		StringSource GetStringSource() const;
	};


// ----- template code -----
template<class... TArgs> StringWrapper::StringWrapper(StringWrapperPrintfTag, const char* s, TArgs&&... args) {
	int len = snprintf(nullptr, 0, s, std::forward<TArgs>(args)...) + 1;
	char* buffer = new char[len];
	snprintf(buffer, len, s, std::forward<TArgs>(args)...);
	data = buffer;
	type = StringWrapperType::st_dynamic_new;
	}


// --------------------------------------------------------------------------------------------------------------------------
//	End 
// --------------------------------------------------------------------------------------------------------------------------

//}			// end of ESPAsync namespace