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
//==============================================================================
namespace xcore
{
    class x_va;
    class x_va_list;
    class xalloc;

    class xstr8
    {
    public:
		xstr8(utf8::alloc* allocator, utf8::runes  str);
		~xstr8();

		uchar8 const*	str() const;

	private:
        utf8::alloc* m_allocator;
        utf8::runes  m_string;
	};

    class xstr16
    {
    public:
		xstr16(utf16::alloc* allocator, utf16::runes  str);
		~xstr16();

		uchar16 const*	str() const;

	private:
        utf16::alloc* m_allocator;
        utf16::runes  m_string;
	};

    class xstring
    {
    public:
        xstring();
        xstring(const char* str);
        xstring(utf32::alloc* allocator, s32 size);
        xstring(xstring const& other);
        xstring(xstring const& left, xstring const& right);
        ~xstring();

        bool is_empty() const;
        s32  size() const;

        xstring operator()(s32 to) const;
        xstring operator()(s32 from, s32 to) const;

        uchar32 operator[](s32 index);
        uchar32 operator[](s32 index) const;

        xstring& operator=(const xstring& other)
        {
            release();
            copy(other.m_string, other.m_allocator);
            return *this;
        }

		xstr8	to_utf8(utf8::alloc* allocator) const;
		xstr16	to_utf16(utf16::alloc* allocator) const;

    private:
        void release();
        void copy(const utf32::runes& other, utf32::alloc* allocator);

        utf32::alloc* m_allocator;
        utf32::runes  m_string;
    };

    bool narrowIfDelimited(xstring& inStr, uchar32 leftdel, uchar32 rightdel);

    bool isUpper(const xstring&);
    bool isLower(const xstring&);
    bool isCapitalized(const xstring&);
    bool isQuoted(const xstring&);
    bool isQuoted(const xstring&, uchar32 inQuote);
    bool isDelimited(const xstring&, uchar32 inLeft, uchar32 inRight);

    uchar32 firstChar(const xstring&);
    uchar32 lastChar(const xstring&);

    bool startsWith(const xstring&, xstring const& inStartStr);
    bool endsWith(const xstring&, xstring const& inEndStr);

    ///@name Search/replace
    bool contains(const xstring& inStr, uchar32 inFind);
    bool contains(const xstring& inStr, const xstring& inFind);
    bool containsOneOf(const xstring& inStr, const xstring& inCharSet);

    ///@name Comparison
    s32  compare(const xstring& inLHS, const xstring& inRHS, bool inCaseSensitive);
    bool isEqual(const xstring& inLHS, const xstring& inRHS);
    bool contains(const xstring& inStr, uchar32 inContains);
    bool contains(const xstring& inStr, const xstring& inContains);
    bool containsAny(const xstring& inStr, const xstring& inAny);

    void repeat(xstring&, xstring const& inStr, s32 inTimes);

    s32 format(xstring&, xstring const& formatString, const x_va_list& args);
    s32 formatAdd(xstring&, xstring const& formatString, const x_va_list& args);

    void replace(xstring& inStr, uchar32 find, uchar32 replace);
    void replace(xstring& inStr, xstring const& inFind, xstring const& inReplace);
    s32  replaceAnyWith(xstring&, xstring const& inAny, uchar32 inWith);

    void insert(xstring&, xstring const& inString);
    void insertAfter(xstring&, xstring const& inFind, xstring const& inString);

    void any_remove(xstring& str, const xstring& inAny);
    
	void remove_after(xstring& str, const xstring& find);
    void remove_after_included(xstring& str, const xstring& find);

    void keep_before(xstring& str, const xstring& find);
    void keep_before_included(xstring& str, const xstring& find);

    void upper(xstring& inStr);
    void lower(xstring& inStr);
    void capitalize(xstring& inStr);
    void capitalize(xstring& inStr, xstring const& inSeperators);

    void trim(xstring&);                                // Trim whitespace from left and right side of xstring
    void trimLeft(xstring&);                            // Trim whitespace from left side of xstring
    void trimRight(xstring&);                           // Trim whitespace from right side of xstring
    void trim(xstring&, uchar32 inChar);                // Trim characters in <inCharSet> from left and right side of xstring
    void trimLeft(xstring&, uchar32 inChar);            // Trim character <inChar> from left side of xstring
    void trimRight(xstring&, uchar32 inChar);           // Trim character <inChar> from right side of xstring
    void trim(xstring&, xstring const& inCharSet);      // Trim characters in <inCharSet> from left and right side of xstring
    void trimLeft(xstring&, xstring const& inCharSet);  // Trim characters in <inCharSet> from left side of xstring
    void trimRight(xstring&, xstring const& inCharSet); // Trim characters in <inCharSet> from right side of xstring
    void trimQuotes(xstring&);                          // Trim double quotes from left and right side of xstring
    void trimQuotes(xstring&, uchar32 quote);           // Trim double quotes from left and right side of xstring
    void trimDelimiters(xstring&, uchar32 inLeft, uchar32 inRight); // Trim delimiters from left and right side of xstring

    void reverse(xstring&);

    bool splitOn(xstring&, uchar32 inChar, xstring& outLeft, xstring& outRight);
    bool splitOn(xstring&, xstring& inStr, xstring& outLeft, xstring& outRight);
    bool splitOnLast(xstring&, uchar32 inChar, xstring& outLeft, xstring& outRight);
    bool splitOnLast(xstring&, xstring& inStr, xstring& outLeft, xstring& outRight);

    xstring copy(const xstring& str);
    void    concatenate(xstring& str, const xstring& c);
    void    concatenate(xstring& str, const xstring& c1, const xstring& c2);

    // Global xstring operators

    inline xstring operator+(const xstring& left, const xstring& right) { return xstring(left, right); }

} // namespace xcore

#endif ///< __XSTRING_STRING_H__