// *************************************************************************************
//	StringSource.h
//
//		Simple read only string view.
//		
//																																		Ryan Boghean
//																																		September 2017
//
// *************************************************************************************

# pragma once

# include <cstdint>
//# include <ostream>		// ??

# include <pgmspace.h>
# include <WString.h>


// --------------------------------------------------------------------------------------------------------------------------
//	ESPAsync namespace
// --------------------------------------------------------------------------------------------------------------------------

//namespace ESPAsync {


// --------------------------------------------------------------------------------------------------------------------------
//	StringSource
//		- simple string view
//		- does not store data, merely used to access it
//		- designed to handle both string literals and PROGMEM strings with similar levels of performance
//
//	implemented in: StringSource.cpp
// --------------------------------------------------------------------------------------------------------------------------

// ----- StringSource -----
class StringSource {

	private:
		const char* begin;
		const char* end;

	public:
		StringSource();
		StringSource(const char* begin, const char* end);

		// character access
		const char* Data() const;													// raw data access (be careful, this may be a progmem string)
		size_t Length() const;
		char operator[](size_t) const;
		operator bool() const;														// returns true if the string is valid (not empty)
		StringSource operator()(size_t offset, size_t n) const;		// returns the sub-string [begin + offset, begin + offset + n), offset or n can be 'too large, and will result in a null string not an error

		// string operations
		void CopyTo(char* dest) const;			// copies the string to dest (assumes dest is large enough to hold the string), does not append a null terminating charcter
		size_t Hash() const;								// returns the hash of the string (uses a simple fnv-1a hash, but should be good enough for simple hash tables and speeding up comparisons)
		size_t IndexOf(char) const;					// searches for the first instance of the character in the string and returns its index (or 0xFFFFFFFF if not found)
		int ToInt() const;									// converts the string to an integer (if the string isn't a valid number, has whitespace, or doesn't fit in an int, the results will be nonsense but the function won't error or fail)
		String ToString() const;							// converts to a dyamic string
	};


// ----- stream out -----
//std::ostream& operator<<(std::ostream&, const StringSource&);		// ??


// ----- comparisons -----
bool operator==(const StringSource& s0, const StringSource& s1);
bool operator!=(const StringSource& s0, const StringSource& s1);
bool operator<(const StringSource& s0, const StringSource& s1);
bool operator<=(const StringSource& s0, const StringSource& s1);
bool operator>(const StringSource& s0, const StringSource& s1);
bool operator>=(const StringSource& s0, const StringSource& s1);

bool operator==(const StringSource& s0, const char* s1);
bool operator!=(const StringSource& s0, const char* s1);
bool operator<(const StringSource& s0, const char* s1);
bool operator<=(const StringSource& s0, const char* s1);
bool operator>(const StringSource& s0, const char* s1);
bool operator>=(const StringSource& s0, const char* s1);

bool operator==(const char* s0, const StringSource& s1);
bool operator!=(const char* s0, const StringSource& s1);
bool operator<(const char* s0, const StringSource& s1);
bool operator<=(const char* s0, const StringSource& s1);
bool operator>(const char* s0, const StringSource& s1);
bool operator>=(const char* s0, const StringSource& s1);

bool operator==(const StringSource& s0, const String& s1);
bool operator!=(const StringSource& s0, const String& s1);
bool operator<(const StringSource& s0, const String& s1);
bool operator<=(const StringSource& s0, const String& s1);
bool operator>(const StringSource& s0, const String& s1);
bool operator>=(const StringSource& s0, const String& s1);

bool operator==(const String& s0, const StringSource& s1);
bool operator!=(const String& s0, const StringSource& s1);
bool operator<(const String& s0, const StringSource& s1);
bool operator<=(const String& s0, const StringSource& s1);
bool operator>(const String& s0, const StringSource& s1);
bool operator>=(const String& s0, const StringSource& s1);

bool operator==(const StringSource& s0, const __FlashStringHelper* s1);
bool operator!=(const StringSource& s0, const __FlashStringHelper* s1);
bool operator<(const StringSource& s0, const __FlashStringHelper* s1);
bool operator<=(const StringSource& s0, const __FlashStringHelper* s1);
bool operator>(const StringSource& s0, const __FlashStringHelper* s1);
bool operator>=(const StringSource& s0, const __FlashStringHelper* s1);

bool operator==(const __FlashStringHelper* s0, const StringSource& s1);
bool operator!=(const __FlashStringHelper* s0, const StringSource& s1);
bool operator<(const __FlashStringHelper* s0, const StringSource& s1);
bool operator<=(const __FlashStringHelper* s0, const StringSource& s1);
bool operator>(const __FlashStringHelper* s0, const StringSource& s1);
bool operator>=(const __FlashStringHelper* s0, const StringSource& s1);


// --------------------------------------------------------------------------------------------------------------------------
//	End 
// --------------------------------------------------------------------------------------------------------------------------

//}			// end of ESPAsync namespace