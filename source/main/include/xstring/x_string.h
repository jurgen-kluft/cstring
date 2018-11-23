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
// xstring, a UTF32 string class
// NOTE: xstring does not have a null terminator!
//==============================================================================
namespace xcore
{
	class x_va;
	class x_va_list;
	class xalloc;

	class xstring
	{
	public:
		xstring();
		xstring(xalloc* mem, s32 size);
		xstring(xstring const& other);
		xstring(xstring const& left, xstring const& right);
		~xstring();

		bool			is_empty() const;
		s32				size() const;

		xstring			operator () (s32 to) const;
		xstring			operator () (s32 from, s32 to) const;

		uchar32 		operator [] (s32 index);
		uchar32 		operator [] (s32 index) const;

		xstring&		operator = (const xstring& other)
		{
			m_slice.release();
			m_slice = other.m_slice.obtain();
			return *this;
		}

		slice			m_slice;

		static slice	s_slice;	// Global slice we use to clone from
	};

	xstring				selectUntil(const xstring& inStr, const xstring& inFind);					// Return selection as string of first occurrence of <inStr> after <inPosition> or xstring.is_empty() if not found
	xstring				selectUntilIncluded(const xstring& inStr, const xstring& inFind);			// Return selection as string of first occurrence of <inStr> after <inPosition> or xstring.is_empty() if not found

	bool				narrowIfDelimited(xstring& inStr, uchar32 leftdel, uchar32 rightdel);		// Return true if str could be narrowed since it was quoted (e.g. str='<quoted>', narrow_if_delimited(str, '<', '>'))

	bool				isUpper(const xstring&);
	bool				isLower(const xstring&);
	bool				isCapitalized(const xstring&);
	bool				isQuoted(const xstring&);
	bool				isQuoted(const xstring&, rune inQuote);
	bool				isDelimited(const xstring&, rune inLeft, rune inRight);

	rune				firstChar(const xstring&);
	rune				lastChar(const xstring&);

	bool				startsWith(const xstring&, xstring const& inStartStr);
	bool				endsWith(const xstring&, xstring const& inEndStr);

	///@name Search/replace
	xstring				find(const xstring& inStr, const xstring& inFind);							// Return selection as string of first occurrence of <inStr> after <inPosition> or xstring.is_empty() if not found
	xstring				rfind(const xstring& inStr, const xstring& inFind);							// Return selection as string of last occurrence of <inChar> on or before <inPosition> or xstring.is_empty() if not found
	xstring				findOneOf(const xstring& inStr, const xstring& inFind);						// Return selection as string of first occurrence of a character in <inCharSet> after <inPosition> or xstring.is_empty() if not found
	xstring				rfindOneOf(const xstring& inStr, const xstring& inFind);					// Return selection as string of last occurrence of a character in <inCharSet> after <inPosition> or xstring.is_empty() if not found

	///@name Comparison
	s32					compare(const xstring& inLHS, const xstring& inRHS);						// Return relationship between strings
	bool				isEqual(const xstring& inLHS, const xstring& inRHS);						// Check if two strings are equal, taking capitalization into account
	bool				contains(const xstring& inStr, const xstring& inContains)					{ return find(inStr, inContains).is_empty() == false; }			//< Check if this xstring contains xstring <inString>

	void				repeat(xstring&, xstring const& inStr, s32 inTimes);

	s32					format(xstring&, xstring const& formatString, const x_va_list& args);
	s32					formatAdd(xstring&, xstring const& formatString, const x_va_list& args);

	void				replace(xstring& inStr, xstring const& inReplace);							// Replace character at <inPosition> with <inString>
	s32					replaceAnyWith(xstring&, xstring const& inAny, rune inWith);				// Replace any character from <inAny> in the xstring with the <inWith> character

	void				insert(xstring&, xstring const& inString);									// Insert inString starting at current position

	void				remove(xstring& remove);													// Remove 'remove' from main slice
	void				find_remove(xstring& str, const xstring& remove);							// Remove 'remove' from 'str'
	void				remove_charset(const xstring& inCharSet);									// Remove characters in <inCharSet> from xstring

	void				upper(xstring& inStr);														// Uppercase all chars in xstring (e.g. "myWord" -> "MYWORD")
	void				lower(xstring& inStr);														// Lowercase all chars in xstring (e.g. "myWord" -> "myword")
	void				capitalize(xstring& inStr);													// Capitalize first char in xstring (e.g. "myWord" -> "Myword")
	void				capitalize(xstring& inStr, xstring const& inSeperators);					// Capitalize first char in words (e.g. "my1stWord my2ndWord" -> "My1stword My2ndword")

	void				trim(xstring& inStr);														// Trim whitespace from left and right side of xstring
	void				trimLeft(xstring& inStr);													// Trim whitespace from left side of xstring
	void				trimRight(xstring&);														// Trim whitespace from right side of xstring
	void				trim(xstring&, rune inChar);												// Trim characters in <inCharSet> from left and right side of xstring
	void				trimLeft(xstring&, rune inChar);											// Trim character <inChar> from left side of xstring
	void				trimRight(xstring&, rune inChar);											// Trim character <inChar> from right side of xstring
	void				trim(xstring&, xstring const& inCharSet);									// Trim characters in <inCharSet> from left and right side of xstring
	void				trimLeft(xstring&, xstring const& inCharSet);								// Trim characters in <inCharSet> from left side of xstring
	void				trimRight(xstring&, xstring const& inCharSet);								// Trim characters in <inCharSet> from right side of xstring
	void				trimQuotes(xstring&);														// Trim double quotes from left and right side of xstring
	void				trimQuotes(xstring&, rune quote);											// Trim double quotes from left and right side of xstring
	void				trimDelimiters(xstring&, rune inLeft, rune inRight);						// Trim delimiters from left and right side of xstring

	void				reverse(xstring&);															// Reverse characters in xstring

	bool				splitOn(xstring&, rune inChar, xstring& outLeft, xstring& outRight);		// Split xstring on first occurrence of <ch>, moves all text after <ch> into <outRight>
	bool				splitOn(xstring&, xstring& inStr, xstring& outLeft, xstring& outRight);		// Split xstring on first occurrence of <ch>, moves all text after <ch> into <outRight>
	bool				rsplitOn(xstring&, rune inChar, xstring& outLeft, xstring& outRight);		// Split xstring on last occurrence of <ch>, moves all text after <ch> into <outRight>
	bool				rsplitOn(xstring&, xstring& inStr, xstring& outLeft, xstring& outRight);	// Split xstring on last occurrence of <ch>, moves all text after <ch> into <outRight>

	xstring				copy(const xstring& str);
	void				concatenate(xstring& str, const xstring& c);


	// Global xstring operators

	inline xstring		operator + (const xstring& left, const xstring& right)
	{
		return xstring(left, right);
	}

}

#endif	///< __XSTRING_STRING_H__