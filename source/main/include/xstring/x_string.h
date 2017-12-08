#ifndef __XSTRING_XSTRING_H__
#define __XSTRING_XSTRING_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

//==============================================================================
// INCLUDES
//==============================================================================
#include "xbase/x_debug.h"
#include "xstring/private/x_string_buffers.h"

//==============================================================================
// xCore namespace
//==============================================================================
namespace xcore
{
	class x_va;
	class x_va_list;
	class x_iallocator;

	class xstringd;			// Dynamic memory string
	class xstringtmp;		// Ring buffer based string
	class xstringc;			// C style string (user supplied 'char*')
	class xstringcc;		// C style const string (user supplied 'const char*')

	template<class T>
	class xstring_const_base
	{
	protected:
							xstring_const_base		()															{ }
							xstring_const_base		(s32 len) : mBuffer(len)									{ }
							xstring_const_base		(const char* str) : mBuffer(str)							{ }
		explicit			xstring_const_base		(const xstring_const_base& str);

							xstring_const_base		(s32 len, const char* str) : mBuffer(len, str)				{ }
							xstring_const_base		(const char* formatString, const x_va_list& args) : mBuffer(formatString, args) { }
							xstring_const_base		(const char* formatString, const x_va& v1, const x_va& v2=x_va::sEmpty, const x_va& v3=x_va::sEmpty, const x_va& v4=x_va::sEmpty, const x_va& v5=x_va::sEmpty, const x_va& v6=x_va::sEmpty, const x_va& v7=x_va::sEmpty, const x_va& v8=x_va::sEmpty, const x_va& v9=x_va::sEmpty, const x_va& v10=x_va::sEmpty) : mBuffer(formatString,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10) { }
							xstring_const_base		(s32 lenA, const char* strA, s32 lenB, const char* strB) : mBuffer(lenA, strA, lenB, strB) { }

		explicit			xstring_const_base		(const xstring& str);
		explicit			xstring_const_base		(const xcstring& str);
		explicit			xstring_const_base		(const xccstring& str);

		T					mBuffer;

	public:

		s32					getLength				(void) const;
		bool				isEmpty					(void) const;

		bool				isUpper                 (void) const;
		bool				isLower                 (void) const;
		bool				isCapitalized           (void) const;
		bool				isQuoted                (void) const;
		bool				isQuoted                (char inQuote) const;
		bool				isDelimited             (char inLeft, char inRight) const;

		char				getAt					(s32 inPosition) const;
		char				firstChar				(void) const;
		char				lastChar				(void) const;

		bool				startsWith				(char inChar) const;
		bool				startsWith				(const char* inStartStr) const;
		bool				endsWith				(char inChar) const;
		bool				endsWith				(const char* inEndStr) const;

		///@name Comparison
		s32					compare					(const char* inRHS, s32 inCharNum = -1) const;				///< Return relationship between strings
		s32					compare					(const xstring& inRHS, s32 inCharNum = -1) const;			///< Return relationship between strings
		s32					compare					(const xstring_tmp& inRHS, s32 inCharNum = -1) const;		///< Return relationship between strings
		s32					compare					(const xcstring& inRHS, s32 inCharNum = -1) const;			///< Return relationship between strings
		s32					compare					(const xccstring& inRHS, s32 inCharNum = -1) const;			///< Return relationship between strings

		s32					compareNoCase			(const char* inRHS, s32 inCharNum = -1) const;				///< Return relationship between strings, not taking capitalization into account
		s32					compareNoCase			(const xstring& inRHS, s32 inCharNum = -1) const;			///< Return relationship between strings, not taking capitalization into account
		s32					compareNoCase			(const xstring_tmp& inRHS, s32 inCharNum = -1) const;		///< Return relationship between strings, not taking capitalization into account
		s32					compareNoCase			(const xcstring& inRHS, s32 inCharNum = -1) const;			///< Return relationship between strings, not taking capitalization into account
		s32					compareNoCase			(const xccstring& inRHS, s32 inCharNum = -1) const;			///< Return relationship between strings, not taking capitalization into account

		bool				isEqual					(const char* inRHS, s32 inCharNum = -1) const;				///< Check if two strings are equal, taking capitalization into account
		bool				isEqual					(const xstring& inRHS, s32 inCharNum = -1) const;			///< Check if two strings are equal, taking capitalization into account
		bool				isEqual					(const xstring_tmp& inRHS, s32 inCharNum = -1) const;		///< Check if two strings are equal, taking capitalization into account
		bool				isEqual					(const xcstring& inRHS, s32 inCharNum = -1) const;			///< Check if two strings are equal, taking capitalization into account
		bool				isEqual					(const xccstring& inRHS, s32 inCharNum = -1) const;			///< Check if two strings are equal, taking capitalization into account
		
		bool				isEqualNoCase			(const char* inRHS, s32 inCharNum = -1) const;				///< Check if two strings are equal, not taking capitalization into account
		bool				isEqualNoCase			(const xstring& inRHS, s32 inCharNum = -1) const;			///< Check if two strings are equal, not taking capitalization into account
		bool				isEqualNoCase			(const xstring_tmp& inRHS, s32 inCharNum = -1) const;		///< Check if two strings are equal, not taking capitalization into account
		bool				isEqualNoCase			(const xcstring& inRHS, s32 inCharNum = -1) const;			///< Check if two strings are equal, not taking capitalization into account
		bool				isEqualNoCase			(const xccstring& inRHS, s32 inCharNum = -1) const;			///< Check if two strings are equal, not taking capitalization into account
			
		///@name Search/replace
		s32					find					(char inChar, s32 inPos=0, s32 inLen=-1) const;				///< Return position of first occurrence of <inChar> after <inPosition> or -1 if not found
		s32					find					(const char* inStr, s32 inPos=0, s32 inLen=-1) const;		///< Return position of first occurrence of <inStr> after <inPosition> or -1 if not found
		s32					findNoCase				(char inChar, s32 inPos=0, s32 inLen=-1) const;				///< Return position of first occurrence of <inChar> after <inPosition> or -1 if not found
		s32					findNoCase				(const char* inStr, s32 inPos=0, s32 inLen=-1) const;		///< Return position of first occurrence of <inStr> after <inPosition> or -1 if not found
		s32					rfind					(char inChar, s32 inPos=-1, s32 inLen=-1) const;			///< Return position of last occurrence of <inChar> on or before <inPosition> or -1 if not found
		s32					rfind					(const char* inStr, s32 inPos=-1, s32 inLen=-1) const;		///< Return position of last occurrence of <inChar> on or before <inPosition> or -1 if not found
		s32					findOneOf				(const char* inCharSet, s32 inPos = 0) const;				///< Return position of first occurrence of a character in <inCharSet> after <inPosition> or -1 if not found
		s32					rfindOneOf				(const char* inCharSet, s32 inPos = -1) const;				///< Return position of last occurrence of a character in <inCharSet> after <inPosition> or -1 if not found

		s32					indexOf					(char inChar, s32 inPosition = 0) const						{ return find(inChar, inPosition); }
		s32					indexOf					(const char* inStr, s32 inPosition = 0) const				{ return find(inStr, inPosition); }
		s32					lastIndexOf				(char inChar, s32 inPosition = -1) const					{ return rfind(inChar, inPosition); }
		s32					lastIndexOf				(const char* inStr, s32 inPosition = -1) const				{ return rfind(inStr, inPosition); }

		bool				contains				(char inChar) const											{ return find(inChar) != -1; }				///< Check if this string contains character <inChar>
		bool				contains				(char inChar, s32 inPos, s32 inLen=-1) const				{ return find(inChar, 0, inLen) != -1; }	///< Check if this string contains character <inChar>
		bool				contains				(const char* inStr) const									{ return find(inStr) != -1; }				///< Check if this string contains string <inString>
		bool				contains				(const char* inStr, s32 inPos, s32 inLen=-1) const			{ return find(inStr, 0, inLen) != -1; }		///< Check if this string contains string <inString>
		bool				containsNoCase			(const char* inStr) const									{ return findNoCase(inStr) != -1; }			///< Check if this string contains <inString> without paying attention to case
		bool				containsNoCase			(const char* inStr, s32 inPos, s32 inLen=-1) const			{ return findNoCase(inStr, inPos, inLen) != -1; } ///< Check if this string contains <inString> without paying attention to case

		// xstring versions
		void				left					(s32 inNum, xstring& outLeft) const;							///< Return the leftmost <inNum> characters of this string
		void				right					(s32 inNum, xstring& outRight) const;							///< Return the rightmost <inNum> characters of this string
		void				mid						(s32 inPosition, xstring& outMid, s32 inNum = -1) const;		///< Return a string containing <inNum> characters from this string, starting at <inPosition>
		void				substring				(s32 inPosition, xstring& outSubstring, s32 inNum) const;
		void				substring				(s32 inPosition, xstring& outSubstring) const;

		bool				splitOn					(const char inChar, xstring& outLeft, xstring& outRight) const;	///< Split string on first of occurrence of <ch>, returns result in <outLeft> and <outRight>
		bool				rsplitOn				(const char inChar, xstring& outLeft, xstring& outRight) const;	///< Split string on last of occurrence of <ch>, returns result in <outLeft> and <outRight>
		void				split					(s32 inPosition, bool inRemove, xstring& outLeft, xstring& outRight) const;	///< Split string at <inPosition>, return results in <outLeft> and <outRight>, if <inRemove> is true result doesn't include tchar at <inPosition>

		// xstring_tmp versions
		void				left					(s32 inNum, xstring_tmp& outLeft) const;							///< Return the leftmost <inNum> characters of this string
		void				right					(s32 inNum, xstring_tmp& outRight) const;							///< Return the rightmost <inNum> characters of this string
		void				mid						(s32 inPosition, xstring_tmp& outMid, s32 inNum = -1) const;		///< Return a string containing <inNum> characters from this string, starting at <inPosition>
		void				substring				(s32 inPosition, xstring_tmp& outSubstring, s32 inNum) const;
		void				substring				(s32 inPosition, xstring_tmp& outSubstring) const;

		bool				splitOn					(const char inChar, xstring_tmp& outLeft, xstring_tmp& outRight) const;	///< Split string on first of occurrence of <ch>, returns result in <outLeft> and <outRight>
		bool				rsplitOn				(const char inChar, xstring_tmp& outLeft, xstring_tmp& outRight) const;	///< Split string on last of occurrence of <ch>, returns result in <outLeft> and <outRight>
		void				split					(s32 inPosition, bool inRemove, xstring_tmp& outLeft, xstring_tmp& outRight) const;	///< Split string at <inPosition>, return results in <outLeft> and <outRight>, if <inRemove> is true result doesn't include tchar at <inPosition>

		// xcstring versions
		void				left					(s32 inNum, xcstring& outLeft) const;							///< Return the leftmost <inNum> characters of this string
		void				right					(s32 inNum, xcstring& outRight) const;							///< Return the rightmost <inNum> characters of this string
		void				mid						(s32 inPosition, xcstring& outMid, s32 inNum = -1) const;		///< Return a string containing <inNum> characters from this string, starting at <inPosition>
		void				substring				(s32 inPosition, xcstring& outSubstring, s32 inNum) const;
		void				substring				(s32 inPosition, xcstring& outSubstring) const;

		bool				splitOn					(const char inChar, xcstring& outLeft, xcstring& outRight) const;	///< Split string on first of occurrence of <ch>, returns result in <outLeft> and <outRight>
		bool				rsplitOn				(const char inChar, xcstring& outLeft, xcstring& outRight) const;	///< Split string on last of occurrence of <ch>, returns result in <outLeft> and <outRight>
		void				split					(s32 inPosition, bool inRemove, xcstring& outLeft, xcstring& outRight) const;	///< Split string at <inPosition>, return results in <outLeft> and <outRight>, if <inRemove> is true result doesn't include tchar at <inPosition>


		inline const char*	c_str					(void) const 												{ return mBuffer.getPointer(); }
	};

	template<class T>
	class xstring_mutable_base : public xstring_const_base<T>
	{
	public:
		typedef				xstring_const_base<T>									__const_base;

	protected:
							xstring_mutable_base	(void);
							xstring_mutable_base	(s32 len);
							xstring_mutable_base	(const char* str);
							xstring_mutable_base	(s32 len, const char* str);
							xstring_mutable_base	(s32 lenA, const char* strA, s32 lenB, const char* strB);
							xstring_mutable_base	(const char* formatString, const x_va_list& args);
							xstring_mutable_base	(const char* formatString, const x_va& v1, const x_va& v2=x_va::sEmpty, const x_va& v3=x_va::sEmpty, const x_va& v4=x_va::sEmpty, const x_va& v5=x_va::sEmpty, const x_va& v6=x_va::sEmpty, const x_va& v7=x_va::sEmpty, const x_va& v8=x_va::sEmpty, const x_va& v9=x_va::sEmpty, const x_va& v10=x_va::sEmpty);

							xstring_mutable_base	(const xstring& str);
							xstring_mutable_base	(const xcstring& str);
							xstring_mutable_base	(const xccstring& str);

		inline char*		w_str					(void)								{ return __const_base::mBuffer.getPointer(); }

	public:

		s32					getMaxLength			(void) const						{ return __const_base::mBuffer.getMaxLength(); }

		void				clear					(void);

		void				repeat					(const char* inString, s32 inTimes);
		void				repeat					(const char* inString, s32 inTimes, s32 inStringLength);
		void				repeat					(const xstring& inString, s32 inTimes);
		void				repeat					(const xcstring& inString, s32 inTimes);
		void				repeat					(const xccstring& inString, s32 inTimes);
		void				repeat					(const xstring_tmp& inString, s32 inTimes);

		s32					format					(const char* formatString, const x_va_list& args);
		s32					formatAdd				(const char* formatString, const x_va_list& args);

		void				setAt					(s32 inPosition, char inChar);											///< Set character <inChar> at <inPosition>
		void				setAt					(s32 inPosition, const char* inString, s32 inNum);						///< Replace character at <inPosition> with <inString>
		void				setAt					(s32 inPosition, s32 inLen, const char* inString, s32 inNum=-1);		///< Replace <inNum> characters at <inPosition> with <inString>

		void				replace					(s32 inPos, const char* inString, s32 inLen=-1);						///< Replace character at <inPosition> with <inString>
		void				replace					(s32 inPos, s32 inLen, const char* inString, s32 inNum=-1);				///< Replace <inLength> characters at <inPosition> with inNumChars characters of <inString>
		s32					replace					(const char* inFind, const char* inSubstitute, s32 inPos=0);			///< Replace all occurrences of string <inFind> after <inPosition> with string <inSubstitute>, returns amount of replacements made
		s32					replace					(char inFind, const char* inSubstitute, s32 inPos=0);					///< Replace all occurrences of character <inFind> after <inPosition> with string <inSubstitute>, returns amount of replacements made
		s32					replace					(const char* inFind, char inSubstitute, s32 inPos=0);					///< Replace all occurrences of string <inFind> after <inPosition> with char <inSubstitute>, returns amount of replacements made
		s32					replace					(char inFind, char inSubstitute, s32 inPos=0);							///< Replace all occurrences of char <inFind> after <inPosition> with char <inSubstitute>, returns amount of replacements made

		s32					replaceAnyWith			(const char* inAny, char inWith, s32 inFrom=0, s32 inNum=-1);			///< Replace any character from <inAny> in the string with the <inWith> character

		void				insert					(char inChar);															///< Insert inChar at current position
		void				insert					(const char* inString, s32 inNum=-1);									///< Insert inString starting at current position
		void				insert					(const xstring& inString);												///< Insert inString starting at current position
		void				insert					(const xcstring& inString);												///< Insert inString starting at current position
		void				insert					(const xccstring& inString);											///< Insert inString starting at current position
		void				insert					(const xstring_tmp& inString);											///< Insert inString starting at current position
		void				insert					(s32 inPos, char inChar);												///< Insert inChar at <inPosition>
		void				insert					(s32 inPos, const char* inString, s32 inNum=-1);						///< Insert inString starting at <inPosition>
		void				insert					(s32 inPos, const xstring& inString);									///< Insert inString starting at <inPosition>
		void				insert					(s32 inPos, const xcstring& inString);									///< Insert inString starting at <inPosition>
		void				insert					(s32 inPos, const xccstring& inString);									///< Insert inString starting at <inPosition>
		void				insert					(s32 inPos, const xstring_tmp& inString);								///< Insert inString starting at <inPosition>

		void				remove					(s32 inStart, s32 inLength);											///< Remove <inLength> characters starting at <inStart>
		void				remove					(const char* inCharSet);												///< Remove characters in <inCharSet> from string

		void				upper					(void);																	///< Uppercase all chars in string (e.g. "myWord" -> "MYWORD")
		void				lower					(void);																	///< Lowercase all chars in string (e.g. "myWord" -> "myword")
		void				capitalize				(void);																	///< Capitalize first char in string (e.g. "myWord" -> "Myword")
		void				capitalize				(const char* inSeperators);												///< Capitalize first char in words (e.g. "my1stWord my2ndWord" -> "My1stword My2ndword")
		void				trim					(void);																	///< Trim whitespace from left and right side of string
		void				trimLeft				(void);																	///< Trim whitespace from left side of string
		void				trimRight				(void);																	///< Trim whitespace from right side of string
		void				trim					(char inChar);															///< Trim characters in <inCharSet> from left and right side of string
		void				trimLeft				(char inChar);															///< Trim character <inChar> from left side of string
		void				trimRight				(char inChar);															///< Trim character <inChar> from right side of string
		void				trim					(const char* inCharSet);												///< Trim characters in <inCharSet> from left and right side of string
		void				trimLeft				(const char* inCharSet);												///< Trim characters in <inCharSet> from left side of string
		void				trimRight				(const char* inCharSet);												///< Trim characters in <inCharSet> from right side of string
		void				trimQuotes				(void);																	///< Trim double quotes from left and right side of string
		void				trimQuotes				(char quote);															///< Trim double quotes from left and right side of string
		void				trimDelimiters			(char inLeft, char inRight);											///< Trim delimiters from left and right side of string
		void				reverse					(void);																	///< Reverse characters in string
		void				reverse					(s32 inPosition, s32 inNum=-1);											///< Reverse characters in a part of this string

		bool				splitOn					(char inChar, xstring& outRight);										///< Split string on first occurrence of <ch>, moves all text after <ch> into <outRight>
		bool				splitOn					(char inChar, xstring& outLeft, xstring& outRight) const;				///< Split string on first occurrence of <ch>, moves all text after <ch> into <outRight>
		bool				rsplitOn				(char inChar, xstring& outRight);										///< Split string on last occurrence of <ch>, moves all text after <ch> into <outRight>
		bool				rsplitOn				(char inChar, xstring& outLeft, xstring& outRight) const;				///< Split string on last occurrence of <ch>, moves all text after <ch> into <outRight>
		void				split					(s32 inPosition, bool inRemove, xstring& outRight);						///< Split string at <inPosition>, move text at the right of inPosition into <outRight>
		void				split					(s32 inPosition, bool inRemove, xstring& outLeft, xstring& outRight) const;	///< Split string at <inPosition>, move text at the right of inPosition into <outRight>

		void				copy					(const char* str);
		void				copy					(const char* str, s32 length);
		void				copy					(const xstring& str);
		void				copy					(const xcstring& str);
		void				copy					(const xccstring& str);
		void				copy					(const xstring_tmp& str);

		void				indexToRowCol			(s32 index, s32& row, s32& col) const;

	protected:
		void				concat					(const char* inSource);													///< Concatenate <inSource> to this string
		void				concat					(const char* inSource, s32 inLength);									///< Concatenate string with <inLength> characters from <inSource>
	};

	typedef		xstring_mutable_base< xstring_buffer_dynamic >		xstringd;
	typedef		xstring_mutable_base< xstring_buffer_char >			xstringc;
	typedef		xstring_const_base< xstring_buffer_const_char >		xstringcc;
	typedef		xstring_mutable_base< xstring_buffer_ring >			xstringtmp;

	#include "xstring/private/x_string_inline.h"
}

#endif	///< __XSTRING_XSTRING_H__