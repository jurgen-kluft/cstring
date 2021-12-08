#ifndef __XSTRING_STRING_H__
#define __XSTRING_STRING_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

//==============================================================================
// INCLUDES
//==============================================================================
#include "xbase/x_debug.h"
#include "xbase/x_runes.h"

//==============================================================================
// xstring, a UTF32 string class
//==============================================================================
namespace xcore
{
    class va_t;
    class va_list_t;
    class alloc_t;

    class xstring
    {
    public:
        xstring();
        xstring(runes_alloc_t* allocator);
        xstring(runes_alloc_t* allocator, const char* str);
        xstring(const char* str);
        xstring(xstring const& other);
        xstring(xstring const& other, xstring const& concat);
        ~xstring();

        bool is_empty() const;
        s32  cap() const;
        s32  size() const;

        void clear();

        xstring operator()(s32 to);
        xstring operator()(s32 from, s32 to);

        xstring operator()(s32 to) const;
        xstring operator()(s32 from, s32 to) const;

        uchar32 operator[](s32 index) const;

        xstring& operator=(const xstring& other);
        xstring& operator+=(const xstring& other);

        bool operator==(const xstring& other) const;
        bool operator!=(const xstring& other) const;

        xstring clone() const;

        static runes_alloc_t* s_allocator;

    protected:
        friend class xview;
        struct data;

        xstring(data*);
        xstring(runes_alloc_t* mem, s32 size);

        void release();
        void clone(runes_t const& str, runes_alloc_t* allocator);

        struct range
        {
            inline range(s32 _from, s32 _to) : from(_from), to(_to) {}
            s32 size() const { return to - from; }
            s32 from;
            s32 to;
        };

        struct view
        {
            inline view() : from(0), to(0) {}
            s32 size() const { return to-from; }
            s32 from;
            s32 to;
        };

        struct data
        {
            inline data() : m_stralloc(nullptr), m_alloc(nullptr), m_str(), m_refs(0), m_list(nullptr) {}
            inline data(runes_alloc_t* sa, alloc_t* a) : m_stralloc(sa), m_alloc(a), m_str(), m_refs(0), m_list(nullptr) {}
            runes_alloc_t* m_stralloc;
            alloc_t* m_alloc;
            runes_t m_str;
            xstring* m_list;
            s32 m_refs;
        };
        data*    m_data;
        xstring* m_next;
        xstring* m_prev;
        view     m_view;
    };

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

    ///@name Comparison
    s32  compare(const xstring& inLHS, const xstring& inRHS);
    bool isEqual(const xstring& inLHS, const xstring& inRHS);
    bool contains(const xstring& inStr, uchar32 inContains);
    bool contains(const xstring& inStr, const xstring& inContains);

    s32 format(xstring&, xstring const& formatString, const va_list_t& args);
    s32 formatAdd(xstring&, xstring const& formatString, const va_list_t& args);

    void upper(xstring& inStr);
    void lower(xstring& inStr);
    void capitalize(xstring& inStr);
    void capitalize(xstring& inStr, xstring const& inSeperators);

    xstring selectUntil(const xstring& inStr, uchar32 inFind);
    xstring selectUntil(const xstring& inStr, const xstring& inFind);

    xstring selectUntilLast(const xstring& inStr, uchar32 inFind);
    xstring selectUntilLast(const xstring& inStr, const xstring& inFind);

    xstring selectUntilIncluded(const xstring& str, const xstring& selection);

    xstring selectUntilEndExcludeSelection(const xstring& str, const xstring& selection);
    xstring selectUntilEndIncludeSelection(const xstring& str, const xstring& selection);

    ///@name Search/replace
    xstring find(xstring& inStr, uchar32 inFind);
    xstring find(xstring& inStr, const xstring& inFind);
    xstring findLast(xstring& inStr, const xstring& inFind);
    xstring findOneOf(xstring& inStr, const xstring& inFind);
    xstring findOneOfLast(xstring& inStr, const xstring& inFind);

    void insert(xstring&, xstring const& pos, xstring const& insert);
    void insert_after(xstring&, xstring const& pos, xstring const& insert);

    void remove(xstring&, xstring const& selection);
    void find_remove(xstring&, const xstring& find);
    void find_replace(xstring&, xstring const& find, xstring const& replace);
    void remove_any(xstring&, const xstring& inAny);
    void replace_any(xstring&, xstring const& inAny, uchar32 inWith);

    void trim(xstring&);
    void trimLeft(xstring&);
    void trimRight(xstring&);
    void trim(xstring&, uchar32 inChar);
    void trimLeft(xstring&, uchar32 inChar);
    void trimRight(xstring&, uchar32 inChar);
    void trim(xstring&, xstring const& inCharSet);
    void trimLeft(xstring&, xstring const& inCharSet);
    void trimRight(xstring&, xstring const& inCharSet);
    void trimQuotes(xstring&);
    void trimQuotes(xstring&, uchar32 quote);
    void trimDelimiters(xstring&, uchar32 inLeft, uchar32 inRight);

    void reverse(xstring&);

    bool splitOn(xstring&, uchar32 inChar, xstring& outLeft, xstring& outRight);
    bool splitOn(xstring&, xstring& inStr, xstring& outLeft, xstring& outRight);
    bool splitOnLast(xstring&, uchar32 inChar, xstring& outLeft, xstring& outRight);
    bool splitOnLast(xstring&, xstring& inStr, xstring& outLeft, xstring& outRight);

    void concatenate(xstring& str, const xstring& con);
    void concatenate_repeat(xstring&, xstring const& con, s32 ntimes);

    // Global xstring operators

    inline xstring operator+(const xstring& left, const xstring& right) { return xstring(left, right); }
} // namespace xcore

#endif ///< __XSTRING_STRING_H__