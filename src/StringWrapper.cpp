// *************************************************************************************
//	StringWrapper.cpp
//
//		StringWrapper.h implementation.
//		
//																																		Ryan Boghean
//																																		September 2017
//
// *************************************************************************************

# pragma once

# include <cstdint>
# include <cstring>
# include <utility>

# include <pgmspace.h>
# include <Wstring.h>

# include "PGMHelper.h"
# include "StringSource.h"
# include "StringWrapper.h"

//using namespace ESPAsync;


// --------------------------------------------------------------------------------------------------------------------------
//	StringWrapper
// --------------------------------------------------------------------------------------------------------------------------

// ----- static data -----
const char* StringWrapper::empty_string = "";


// ----- standard member functions -----
StringWrapper::StringWrapper() {
	data = empty_string;
	type = StringWrapperType::st_literal;
	}

StringWrapper::StringWrapper(const String& s) : StringWrapper() {
	AssignString(s);
	}

StringWrapper::StringWrapper(String&& s) : StringWrapper() {
	MoveString(std::move(s));
	}

StringWrapper::StringWrapper(const __FlashStringHelper* s) : StringWrapper() {
	if (s) {
		data = reinterpret_cast<const char*>(s);
		type = StringWrapperType::st_progmem;
		}
	}

StringWrapper::StringWrapper(const char* s, StringWrapperType t) : StringWrapper() {
	if (s) {
		data = s;
		type = t;
		}
	}

StringWrapper::StringWrapper(StringWrapper&& s) {
	data = s.data; s.data = empty_string;
	type = s.type; s.type = StringWrapperType::st_literal;
	}

StringWrapper& StringWrapper::operator=(StringWrapper&& s) {
	if (this == &s) return *this;
	data = s.data; s.data = empty_string;
	type = s.type; s.type = StringWrapperType::st_literal;
	return *this;
	}

StringWrapper& StringWrapper::operator=(const String& s) {
	Clear();
	AssignString(s);
	return *this;
	}

StringWrapper& StringWrapper::operator=(String&& s) {
	Clear();
	MoveString(std::move(s));
	return *this;
	}

StringWrapper& StringWrapper::operator=(const __FlashStringHelper* s) {
	Clear();
	if (s) {
		data = reinterpret_cast<const char*>(s);
		type = StringWrapperType::st_progmem;
		}
	return *this;
	}

StringWrapper::~StringWrapper() {
	Clear();
	}


// ----- internal helpers -----
void StringWrapper::AssignString(const String& s) {
	// assumes StringWrapper is clear/empty

	unsigned int len = s.length();
	if (len == 0) return;

	if (len) {
		char* new_data = new char[len + 1];
		std::memcpy(new_data, s.c_str(), len);
		new_data[len] = 0;
		data = new_data;
		type = StringWrapperType::st_dynamic_new;
		}
	else {
		data = empty_string;
		type = StringWrapperType::st_literal;
		}
	}

void StringWrapper::MoveString(String&& s) {
	// sadly I can't think of any non-hackish way to steal the string buffer...
	// I can think of a few hackish ways if necessary
	AssignString(s);
	}


// ----- helpers -----
void StringWrapper::Clear() {

	switch (type) {
		case StringWrapperType::st_dynamic_new:
			delete[] const_cast<char*>(data);
			break;

		case StringWrapperType::	st_dynamic_malloc:
			if (data) free(const_cast<char*>(data));
			break;
		}

	data = empty_string;
	type = StringWrapperType::st_literal;
	}

const char* StringWrapper::Data() const {
	return data;
	}

StringWrapperType StringWrapper::Type() const {
	return type;
	}

StringSource StringWrapper::GetStringSource() const {
	return { data, data + StrLen(data) };
	}