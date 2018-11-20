#ifndef __XSTRING_STRING_H__
#define __XSTRING_STRING_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

//==============================================================================
// INCLUDES
//==============================================================================
#include "xbase/x_debug.h"
#include "xbase/x_slice.h"

//==============================================================================
// string_t, a UTF32 string class
//==============================================================================
namespace xcore
{
	class x_va;
	class x_va_list;
	class xalloc;

	class string_t
	{
	public:
		string_t();
		string_t(xalloc* mem, s32 size);
		string_t(string_t const& other);
		~string_t();

		bool			is_empty() const;
		s32				size() const;

		string_t		operator () (s32 to) const;
		string_t		operator () (s32 from, s32 to) const;

		uchar32 		operator [] (s32 index);
		uchar32 		operator [] (s32 index) const;

		string_t&		operator = (const string_t& other)
		{
			m_slice.release();
			m_slice = other.m_slice.obtain();
			return *this;
		}

		string_t&		operator += (const string_t& other)
		{
			concatenate(*this, other);
			return *this;
		}

		slice			m_slice;
	};

	static void user_case_for_string()
	{
		string_t str;
		string_t substr = str(0, 10);
	}

	bool				isUpper(const string_t&);
	bool				isLower(const string_t&);
	bool				isCapitalized(const string_t&);
	bool				isQuoted(const string_t&);
	bool				isQuoted(const string_t&, rune inQuote);
	bool				isDelimited(const string_t&, rune inLeft, rune inRight);

	rune				firstChar(const string_t&);
	rune				lastChar(const string_t&);

	bool				startsWith(const string_t&, crunes const& inStartStr);
	bool				endsWith(const string_t&, crunes const& inEndStr);

	///@name Comparison
	s32					compare(const string_t& inLHS, const string_t& inRHS);							// Return relationship between strings
	s32					compareNoCase(const string_t& inLHS, const string_t& inRHS);					// Return relationship between strings, not taking capitalization into account
	bool				isEqual(const string_t& inLHS, const string_t& inRHS);							// Check if two strings are equal, taking capitalization into account
	bool				isEqualNoCase(const string_t& inLHS, const string_t& inRHS);					// Check if two strings are equal, not taking capitalization into account
	bool				contains(const string_t& inStr, const string_t& inContains)						{ return find(inStr, inContains).is_empty() == false; }				//< Check if this string_t contains string_t <inString>
	bool				containsNoCase(const string_t& inStr, const string_t& inContains)				{ return findNoCase(inStr, inContains).is_empty() == false; }	//< Check if this string_t contains <inString> without paying attention to case

	///@name Search/replace
	string_t			find(const string_t& inStr, const string_t& inFind);							// Return selection as string of first occurrence of <inStr> after <inPosition> or string_t.is_empty() if not found
	string_t			findNoCase(const string_t& inStr, const string_t& inFind);						// Return selection as string of first occurrence of <inStr> after <inPosition> or string_t.is_empty() if not found
	string_t			rfind(const string_t& inStr, const string_t& inFind);							// Return selection as string of last occurrence of <inChar> on or before <inPosition> or string_t.is_empty() if not found
	string_t			rfindNoCase(const string_t& inStr, const string_t& inFind);						// Return selection as string of last occurrence of <inChar> on or before <inPosition> or string_t.is_empty() if not found
	string_t			findOneOf(const string_t& inStr, const string_t& inFind);						// Return selection as string of first occurrence of a character in <inCharSet> after <inPosition> or string_t.is_empty() if not found
	string_t			rfindOneOf(const string_t& inStr, const string_t& inFind);						// Return selection as string of last occurrence of a character in <inCharSet> after <inPosition> or string_t.is_empty() if not found

	void				repeat(const string_t&, string_t const& inStr, s32 inTimes);

	s32					format(const string_t&, string_t const& formatString, const x_va_list& args);
	s32					formatAdd(const string_t&, string_t const& formatString, const x_va_list& args);

	void				replace(string_t& inStr, string_t const& inReplace);							// Replace character at <inPosition> with <inString>
	s32					replaceAnyWith(string_t&, string_t const& inAny, char inWith);					// Replace any character from <inAny> in the string_t with the <inWith> character

	void				insert(string_t&, string_t const& inString);									// Insert inString starting at current position

	void				remove(string_t& remove);														// Remove 'remove' from main slice
	void				find_remove(string_t& str, const string_t& remove);								// Remove 'remove' from 'str'
	void				remove_charset(const string_t& inCharSet);										// Remove characters in <inCharSet> from string_t

	void				upper(string_t& inStr);															// Uppercase all chars in string_t (e.g. "myWord" -> "MYWORD")
	void				lower(string_t& inStr);															// Lowercase all chars in string_t (e.g. "myWord" -> "myword")
	void				capitalize(string_t& inStr);													// Capitalize first char in string_t (e.g. "myWord" -> "Myword")
	void				capitalize(string_t& inStr, crunes const& inSeperators);						// Capitalize first char in words (e.g. "my1stWord my2ndWord" -> "My1stword My2ndword")
	void				trim(string_t& inStr);															// Trim whitespace from left and right side of string_t
	void				trimLeft(string_t& inStr);														// Trim whitespace from left side of string_t
	void				trimRight(string_t&);															// Trim whitespace from right side of string_t
	void				trim(string_t&, rune inChar);													// Trim characters in <inCharSet> from left and right side of string_t
	void				trimLeft(string_t&, rune inChar);												// Trim character <inChar> from left side of string_t
	void				trimRight(string_t&, rune inChar);												// Trim character <inChar> from right side of string_t
	void				trim(string_t&, crunes const& inCharSet);										// Trim characters in <inCharSet> from left and right side of string_t
	void				trimLeft(string_t&, crunes const& inCharSet);									// Trim characters in <inCharSet> from left side of string_t
	void				trimRight(string_t&, crunes const& inCharSet);									// Trim characters in <inCharSet> from right side of string_t
	void				trimQuotes(string_t&);															// Trim double quotes from left and right side of string_t
	void				trimQuotes(string_t&, rune quote);												// Trim double quotes from left and right side of string_t
	void				trimDelimiters(string_t&, rune inLeft, rune inRight);							// Trim delimiters from left and right side of string_t
	void				reverse(string_t&);																// Reverse characters in string_t

	bool				splitOn(string_t&, char inChar, string_t& outLeft, string_t& outRight);			// Split string_t on first occurrence of <ch>, moves all text after <ch> into <outRight>
	bool				splitOn(string_t&, string_t& inStr, string_t& outLeft, string_t& outRight);		// Split string_t on first occurrence of <ch>, moves all text after <ch> into <outRight>
	bool				rsplitOn(string_t&, char inChar, string_t& outLeft, string_t& outRight);		// Split string_t on last occurrence of <ch>, moves all text after <ch> into <outRight>
	bool				rsplitOn(string_t&, string_t& inStr, string_t& outLeft, string_t& outRight);	// Split string_t on last occurrence of <ch>, moves all text after <ch> into <outRight>

	string_t			copy(const string_t& src);
	void				concatenate(string_t& str, const string_t& );
}

#endif	///< __XSTRING_STRING_H__