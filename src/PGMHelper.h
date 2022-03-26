// *************************************************************************************
//	PGMHelper.h
//
//		Some simple Progmem helper functions.
//		Safe to use for both normal strings and PGM strings.
//		
//																																		Ryan Boghean
//																																		September 2017
//
// *************************************************************************************

# pragma once

# include <cstdint>

# include <pgmspace.h>


// --------------------------------------------------------------------------------------------------------------------------
//	Min/Max functions
//		- the std ones conflict with the macros
//		- this was the easiest way to get around that mess
// --------------------------------------------------------------------------------------------------------------------------

inline uint32_t Min(uint32_t x, uint32_t y) {
	return (x < y) ? x : y;
	}

inline uint32_t Max(uint32_t x, uint32_t y) {
	return (x > y) ? x : y;
	}

template<class T> T Min(T x, T y) {
	return (x < y) ? x : y;
	}

template<class T> T Max(T x, T y) {
	return (x > y) ? x : y;
	}


// --------------------------------------------------------------------------------------------------------------------------
//	IteratePGM
//		- efficient iteration over a PGM string (valid for normal strings as well)
//		- should be faster than pgmspace functions, since pgmspace functions work with only 1 byte at a time,
//			while IteratePGM() works 4 bytes at a time
//		- designed to have similar(ish) levels of performance for both normal and PGM strings
//		- expects a callable object in the form bool(char) or bool(char, char)
//		- iterates over the range [src, src + n), calling f() once for each character
//		- return false to halt execution, true to continue
// --------------------------------------------------------------------------------------------------------------------------

// ----- IteratePGM -----
// iterates over src, only processes 'n' items max, returns the last value of src
template<class TF> const char* IteratePGM(const char* src, size_t n, TF&& f) {
	static constexpr uintptr_t mask = 3;
	static constexpr uintptr_t not_mask = ~mask;

	if (n == 0) return src;
	const char* src_end = src + n;

	// small strings
	if (n < 4) {
		while (src < src_end && f(pgm_read_byte(src))) {
			++src;
			}
		return src;
		}

	// handle initial dword
	const uint32_t* dword_ptr = reinterpret_cast<const uint32_t*>(reinterpret_cast<uintptr_t>(src) & not_mask);
	uintptr_t rem = reinterpret_cast<uintptr_t>(src) & mask;
	uint32_t dword = *dword_ptr++;
	switch (rem) {
		case 0: if (!f(static_cast<char>(dword))) return src; ++src;
		case 1: if (!f(static_cast<char>(dword >> 8))) return src; ++src;
		case 2: if (!f(static_cast<char>(dword >> 16))) return src; ++src;
		case 3: if (!f(static_cast<char>(dword >> 24))) return src; ++src;
		}

	// middle dwords
	const uint32_t* dword_end = reinterpret_cast<const uint32_t*>(reinterpret_cast<uintptr_t>(src_end) & not_mask);
	while (dword_ptr < dword_end) {
		dword = *dword_ptr++;
		if (!f(static_cast<char>(dword))) return src; ++src;
		if (!f(static_cast<char>(dword >> 8))) return src; ++src;
		if (!f(static_cast<char>(dword >> 16))) return src; ++src;
		if (!f(static_cast<char>(dword >> 24))) return src; ++src;
		}

	// final dword
	uintptr_t rem_end = reinterpret_cast<uintptr_t>(src_end) & mask;
	if (rem_end) {
		dword = *dword_ptr;
		for (uintptr_t i = 0; i < rem_end; ++i) {
			if (!f(static_cast<char>(dword >> (i * 8)))) return src; ++src;
			}
		}
	}


// ----- IteratePGM -----
// iterates over src, relies on f() returing false to halt execution, returns the last value of src
template<class TF> const char* IteratePGM(const char* src, TF&& f) {
	static constexpr uintptr_t mask = 3;
	static constexpr uintptr_t not_mask = ~mask;

	const uint32_t* dword_ptr = reinterpret_cast<const uint32_t*>(reinterpret_cast<uintptr_t>(src) & not_mask);
	uintptr_t rem = reinterpret_cast<uintptr_t>(src) & mask;

	uint32_t dword = *dword_ptr++;
	switch (rem) {
		case 0: if (!f(static_cast<char>(dword))) return src; ++src;
		case 1: if (!f(static_cast<char>(dword >> 8))) return src; ++src;
		case 2: if (!f(static_cast<char>(dword >> 16))) return src; ++src;
		case 3: if (!f(static_cast<char>(dword >> 24))) return src; ++src;
		}

	while (true) {
		dword = *dword_ptr++;
		if (!f(static_cast<char>(dword))) return src; ++src;
		if (!f(static_cast<char>(dword >> 8))) return src; ++src;
		if (!f(static_cast<char>(dword >> 16))) return src; ++src;
		if (!f(static_cast<char>(dword >> 24))) return src; ++src;
		}
	}


// ----- IteratePGM -----
// iterates over src0 and src1 simutaneously, calling f() once for each pair
// relies on f() returing false to halt execution
template<class TF> void IteratePGM(const char* src0, const char* src1, TF&& f) {
	static constexpr uintptr_t mask = 3;
	static constexpr uintptr_t not_mask = ~mask;

	// initialize
	const uint32_t* dword_ptr0 = reinterpret_cast<const uint32_t*>(reinterpret_cast<uintptr_t>(src0) & not_mask);
	const uint32_t* dword_ptr1 = reinterpret_cast<const uint32_t*>(reinterpret_cast<uintptr_t>(src1) & not_mask);
	uintptr_t rem0 = reinterpret_cast<uintptr_t>(src0) & mask;
	uintptr_t rem1 = reinterpret_cast<uintptr_t>(src1) & mask;
	uint32_t dword0 = (*dword_ptr0++) >> (rem0 * 8);
	uint32_t dword1 = (*dword_ptr1++) >> (rem1 * 8);

	// jump table
	switch ( (rem0 << 4 | rem1) ) {
		case 0x00: goto label_00;
		case 0x01: goto label_01;
		case 0x02: goto label_02;
		case 0x03: goto label_03;
		case 0x10: goto label_10;
		case 0x11: goto label_11;
		case 0x12: goto label_12;
		case 0x13: goto label_13;
		case 0x20: goto label_20;
		case 0x21: goto label_21;
		case 0x22: goto label_22;
		case 0x23: goto label_23;
		case 0x30: goto label_30;
		case 0x31: goto label_31;
		case 0x32: goto label_32;
		case 0x33: goto label_33;
		}
	
	// main loop
	while (true) {

		dword0 = *dword_ptr0++;
		dword1 = *dword_ptr1++;

		label_00:
		if (!f(static_cast<char>(dword0), static_cast<char>(dword1))) return;
		dword0 >>= 8;
		dword1 >>= 8;

		label_11:
		if (!f(static_cast<char>(dword0), static_cast<char>(dword1))) return;
		dword0 >>= 8;
		dword1 >>= 8;

		label_22:
		if (!f(static_cast<char>(dword0), static_cast<char>(dword1))) return;
		dword0 >>= 8;
		dword1 >>= 8;

		label_33:
		if (!f(static_cast<char>(dword0), static_cast<char>(dword1))) return;
		dword0 >>= 8;
		dword1 >>= 8;
		}

	while (true) {

		dword0 = *dword_ptr0++;

		label_03:
		if (!f(static_cast<char>(dword0), static_cast<char>(dword1))) return;
		dword0 >>= 8;
		dword1 >>= 8;

		dword1 = *dword_ptr1++;

		label_10:
		if (!f(static_cast<char>(dword0), static_cast<char>(dword1))) return;
		dword0 >>= 8;
		dword1 >>= 8;

		label_21:
		if (!f(static_cast<char>(dword0), static_cast<char>(dword1))) return;
		dword0 >>= 8;
		dword1 >>= 8;

		label_32:
		if (!f(static_cast<char>(dword0), static_cast<char>(dword1))) return;
		dword0 >>= 8;
		dword1 >>= 8;
		}

	while (true) {

		dword0 = *dword_ptr0++;	

		label_02:
		if (!f(static_cast<char>(dword0), static_cast<char>(dword1))) return;
		dword0 >>= 8;
		dword1 >>= 8;

		label_13:
		if (!f(static_cast<char>(dword0), static_cast<char>(dword1))) return;
		dword0 >>= 8;
		dword1 >>= 8;

		dword1 = *dword_ptr1++;

		label_20:
		if (!f(static_cast<char>(dword0), static_cast<char>(dword1))) return;
		dword0 >>= 8;
		dword1 >>= 8;

		label_31:
		if (!f(static_cast<char>(dword0), static_cast<char>(dword1))) return;
		dword0 >>= 8;
		dword1 >>= 8;
		}

	while (true) {

		dword0 = *dword_ptr0++;	

		label_01:
		if (!f(static_cast<char>(dword0), static_cast<char>(dword1))) return;
		dword0 >>= 8;
		dword1 >>= 8;

		label_12:
		if (!f(static_cast<char>(dword0), static_cast<char>(dword1))) return;
		dword0 >>= 8;
		dword1 >>= 8;

		label_23:
		if (!f(static_cast<char>(dword0), static_cast<char>(dword1))) return;
		dword0 >>= 8;
		dword1 >>= 8;

		dword1 = *dword_ptr1++;

		label_30:
		if (!f(static_cast<char>(dword0), static_cast<char>(dword1))) return;
		dword0 >>= 8;
		dword1 >>= 8;
		}
	}


// --------------------------------------------------------------------------------------------------------------------------
//	More helpers
//		- these should (in theory, not yet tested) be faster than the pgmspace.h versions, as they process 4
//			byes at a time
// --------------------------------------------------------------------------------------------------------------------------

// ----- StrLen -----
inline size_t StrLen(const char* s) {
	const char* e = IteratePGM(s,
		[](char c) { return c != 0; }
		);
	return e - s;
	}


// ----- StrCmp -----
inline int StrCmp(const char* s0, const char* s1) {
	int ret;
	IteratePGM(s0, s1,
		[&ret](char c, char d) -> bool {
			if (c == 0 || d == 0 || c != d) { ret = static_cast<int>(c) - static_cast<int>(d); return false; }
			return true;
			}
		);
	return ret;
	}

inline int StrCmp(const char* s0, size_t len0, const char* s1, size_t len1) {
	size_t len = Min(len0, len1);
	if (len == 0) return static_cast<int>(len0) - static_cast<int>(len1);

	int ret;
	IteratePGM(s0, s1,
		[&len, &ret, len0, len1](char c, char d) -> bool {
			if (c != d) { ret = static_cast<int>(c) - static_cast<int>(d); return false; }
			if (--len == 0) { ret = static_cast<int>(len0) - static_cast<int>(len1); return false; }
			return true;
			}
		);
	return ret;
	}

inline int StrCmp(const char* s0, size_t len0, const char* s1) {
	return StrCmp(s0, len0, s1, StrLen(s1));		// sadly there's no easy way (that I can think of) to combine null terminated and length terminated strings into a single pass
	}

inline int StrCmp(const char* s0, const char* s1, size_t len1) {
	return StrCmp(s0, StrLen(s0), s1, len1);
	}



// --------------------------------------------------------------------------------------------------------------------------
//	End 
// --------------------------------------------------------------------------------------------------------------------------
