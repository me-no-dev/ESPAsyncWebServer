// *************************************************************************************
//	StringSource.cpp
//
//		StringSource.h implementation.
//		
//																																		Ryan Boghean
//																																		September 2017
//
// *************************************************************************************

# pragma once

# include <cstdint>

# include <pgmspace.h>
# include <Wstring.h>

# include "PGMHelper.h"
# include "StringSource.h"

//using namespace ESPAsync;


// --------------------------------------------------------------------------------------------------------------------------
//	StringSource
// --------------------------------------------------------------------------------------------------------------------------

StringSource::StringSource() {
	begin = nullptr;
	end = nullptr;
	}
	
StringSource::StringSource(const char* begin_, const char* end_) {
	begin = begin_;
	end = end_;
	}


// ----- character access -----
const char* StringSource::Data() const {
	return begin;
	}

size_t StringSource::Length() const {
	return static_cast<size_t>(end - begin);
	}

char StringSource::operator[](size_t i) const {
	return pgm_read_byte(begin + i);
	}

StringSource::operator bool() const {
	return begin != end;
	}

StringSource StringSource::operator()(size_t offset, size_t n) const {
	return {
		Min(begin + offset, end),
		Min(begin + offset + n, end)
		};
	}

// ----- string operations -----
void StringSource::CopyTo(char* dest) const {
	IteratePGM(begin, Length(),
		[&dest](char c) -> bool { *dest++ = c; return true; }
		);
	}

size_t StringSource::Hash() const {
	static constexpr uint32_t fnv_offset = 0x811c9dc5;
	static constexpr uint32_t fnv_prime = 16777619;
	uint32_t hash = fnv_offset;
	IteratePGM(begin, Length(),
		[&hash](char c) -> bool { hash ^= static_cast<uint32_t>(c); hash *= fnv_prime; return true; }
		);
	return hash;
	}

size_t StringSource::IndexOf(char token) const {
	const char* i = IteratePGM(begin, Length(),
		[token](char c) -> bool { return c != token; }
		);
	return static_cast<size_t>(i - begin);
	}

int StringSource::ToInt() const {
	bool sign = false;		// true for negative numbers
	int result = 0;
	IteratePGM(begin, Length(),
		[&result, &sign](char c) -> bool { 
			if (c == '-') sign = true;
			if (c >= 48 && c <= 57) result += static_cast<int>(c) - 48;
			return true;
			}
		);
	return (sign) ? -result : result;
	}

String StringSource::ToString() const {
	String str;
	str.reserve(Length());
	IteratePGM(begin, Length(),
		[&str](char c) -> bool {
			str.concat(c);
			return true;
			}
		);
	return str;
	}


// ----- stream out -----
/*
std::ostream& operator<<(std::ostream& out, const StringSource& str) {
	IteratePGM(str.Data(), str.Length(),
		[&out](char c) -> bool {
			out << c;
			return true;
			}
		);
	return out;
	}*/


// ----- comparisons -----
bool operator==(const StringSource& s0, const StringSource& s1) {
	return StrCmp(s0.Data(), s0.Length(), s1.Data(), s1.Length()) == 0;
	}

bool operator!=(const StringSource& s0, const StringSource& s1) {
	return StrCmp(s0.Data(), s0.Length(), s1.Data(), s1.Length()) != 0;
	}

bool operator<(const StringSource& s0, const StringSource& s1) {
	return StrCmp(s0.Data(), s0.Length(), s1.Data(), s1.Length()) < 0;
	}

bool operator<=(const StringSource& s0, const StringSource& s1) {
	return StrCmp(s0.Data(), s0.Length(), s1.Data(), s1.Length()) <= 0;
	}

bool operator>(const StringSource& s0, const StringSource& s1) {
	return StrCmp(s0.Data(), s0.Length(), s1.Data(), s1.Length()) > 0;
	}

bool operator>=(const StringSource& s0, const StringSource& s1) {
	return StrCmp(s0.Data(), s0.Length(), s1.Data(), s1.Length()) >= 0;
	}

// const char*
bool operator==(const StringSource& s0, const char* s1) {
	return StrCmp(s0.Data(), s0.Length(), s1) == 0;
	}

bool operator!=(const StringSource& s0, const char* s1) {
	return StrCmp(s0.Data(), s0.Length(), s1) != 0;
	}

bool operator<(const StringSource& s0, const char* s1) {
	return StrCmp(s0.Data(), s0.Length(), s1) < 0;
	}

bool operator<=(const StringSource& s0, const char* s1) {
	return StrCmp(s0.Data(), s0.Length(), s1) <= 0;
	}

bool operator>(const StringSource& s0, const char* s1) {
	return StrCmp(s0.Data(), s0.Length(), s1) > 0;
	}

bool operator>=(const StringSource& s0, const char* s1) {
	return StrCmp(s0.Data(), s0.Length(), s1) >= 0;
	}

bool operator==(const char* s0, const StringSource& s1) {
	return StrCmp(s0, s1.Data(), s1.Length()) == 0;
	}

bool operator!=(const char* s0, const StringSource& s1) {
	return StrCmp(s0, s1.Data(), s1.Length()) != 0;
	}

bool operator<(const char* s0, const StringSource& s1) {
	return StrCmp(s0, s1.Data(), s1.Length()) < 0;
	}

bool operator<=(const char* s0, const StringSource& s1) {
	return StrCmp(s0, s1.Data(), s1.Length()) <= 0;
	}

bool operator>(const char* s0, const StringSource& s1) {
	return StrCmp(s0, s1.Data(), s1.Length()) > 0;
	}

bool operator>=(const char* s0, const StringSource& s1) {
	return StrCmp(s0, s1.Data(), s1.Length()) >= 0;
	}

// String
bool operator==(const StringSource& s0, const String& s1) {
	return StrCmp(s0.Data(), s0.Length(), s1.c_str()) == 0;
	}

bool operator!=(const StringSource& s0, const String& s1) {
	return StrCmp(s0.Data(), s0.Length(), s1.c_str()) != 0;
	}

bool operator<(const StringSource& s0, const String& s1) {
	return StrCmp(s0.Data(), s0.Length(), s1.c_str()) < 0;
	}

bool operator<=(const StringSource& s0, const String& s1) {
	return StrCmp(s0.Data(), s0.Length(), s1.c_str()) <= 0;
	}

bool operator>(const StringSource& s0, const String& s1) {
	return StrCmp(s0.Data(), s0.Length(), s1.c_str()) > 0;
	}

bool operator>=(const StringSource& s0, const String& s1) {
	return StrCmp(s0.Data(), s0.Length(), s1.c_str()) >= 0;
	}

bool operator==(const String& s0, const StringSource& s1) {
	return StrCmp(s0.c_str(), s1.Data(), s1.Length()) == 0;
	}

bool operator!=(const String& s0, const StringSource& s1) {
	return StrCmp(s0.c_str(), s1.Data(), s1.Length()) != 0;
	}

bool operator<(const String& s0, const StringSource& s1) {
	return StrCmp(s0.c_str(), s1.Data(), s1.Length()) < 0;
	}

bool operator<=(const String& s0, const StringSource& s1) {
	return StrCmp(s0.c_str(), s1.Data(), s1.Length()) <= 0;
	}

bool operator>(const String& s0, const StringSource& s1) {
	return StrCmp(s0.c_str(), s1.Data(), s1.Length()) > 0;
	}

bool operator>=(const String& s0, const StringSource& s1) {
	return StrCmp(s0.c_str(), s1.Data(), s1.Length()) >= 0;
	}

// __FlashStringHelper
bool operator==(const StringSource& s0, const __FlashStringHelper* s1) {
	return StrCmp(s0.Data(), s0.Length(), reinterpret_cast<const char*>(s1)) == 0;
	}

bool operator!=(const StringSource& s0, const __FlashStringHelper* s1) {
	return StrCmp(s0.Data(), s0.Length(), reinterpret_cast<const char*>(s1)) != 0;
	}

bool operator<(const StringSource& s0, const __FlashStringHelper* s1) {
	return StrCmp(s0.Data(), s0.Length(), reinterpret_cast<const char*>(s1)) < 0;
	}

bool operator<=(const StringSource& s0, const __FlashStringHelper* s1) {
	return StrCmp(s0.Data(), s0.Length(), reinterpret_cast<const char*>(s1)) <= 0;
	}

bool operator>(const StringSource& s0, const __FlashStringHelper* s1) {
	return StrCmp(s0.Data(), s0.Length(), reinterpret_cast<const char*>(s1)) > 0;
	}

bool operator>=(const StringSource& s0, const __FlashStringHelper* s1) {
	return StrCmp(s0.Data(), s0.Length(), reinterpret_cast<const char*>(s1)) >= 0;
	}

bool operator==(const __FlashStringHelper* s0, const StringSource& s1) {
	return StrCmp(reinterpret_cast<const char*>(s0), s1.Data(), s1.Length()) == 0;
	}

bool operator!=(const __FlashStringHelper* s0, const StringSource& s1) {
	return StrCmp(reinterpret_cast<const char*>(s0), s1.Data(), s1.Length()) != 0;
	}

bool operator<(const __FlashStringHelper* s0, const StringSource& s1) {
	return StrCmp(reinterpret_cast<const char*>(s0), s1.Data(), s1.Length()) < 0;
	}

bool operator<=(const __FlashStringHelper* s0, const StringSource& s1) {
	return StrCmp(reinterpret_cast<const char*>(s0), s1.Data(), s1.Length()) <= 0;
	}

bool operator>(const __FlashStringHelper* s0, const StringSource& s1) {
	return StrCmp(reinterpret_cast<const char*>(s0), s1.Data(), s1.Length()) > 0;
	}

bool operator>=(const __FlashStringHelper* s0, const StringSource& s1) {
	return StrCmp(reinterpret_cast<const char*>(s0), s1.Data(), s1.Length()) >= 0;
	}
