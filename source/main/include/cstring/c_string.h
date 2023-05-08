#ifndef __CSTRING_STRING_H__
#define __CSTRING_STRING_H__
#include "cbase/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "cbase/c_debug.h"
#include "cbase/c_runes.h"

namespace ncore
{
    class va_t;
    class va_list_t;
    class alloc_t;

    struct str_data_t;

    class string_t
    {
    public:
        string_t();
        //@TODO: We should also add wchar_t (utf16)
        string_t(const char* str);
        string_t(s32 _len, s32 _type);
        string_t(string_t const& other); // copy constructor
        string_t(string_t const& other, string_t const& concat);
        ~string_t();

        s32 format(string_t const& format, const va_t* argv, s32 argc);
        s32 formatAdd(string_t const& format, const va_t* argv, s32 argc);

        s32      cap() const;
        s32      size() const;
        string_t clone() const;
        bool     is_empty() const;

        void     clear();
        string_t slice() const;

        string_t operator()(s32 from) const;
        string_t operator()(s32 from, s32 to) const;
        uchar32  operator[](s32 index) const;

        string_t& operator=(const char* other);
        string_t& operator=(const string_t& other);
        string_t& operator+=(const string_t& other);

        bool operator==(const string_t& other) const;
        bool operator!=(const string_t& other) const;

    protected:
        friend class str_data_t;
        friend class ustring_t;

        string_t(str_data_t* data, s32 from, s32 to);

        void attach(string_t& str);
        void release();
        void clone(string_t const& str);

        void add_to_list(string_t* node);
        void rem_from_list();
        void invalidate();

        s32                 m_from;
        s32                 m_to;
        string_t*           m_next;
        string_t*           m_prev;
        mutable str_data_t* m_data;
    };

    bool isUpper(const string_t&);
    bool isLower(const string_t&);
    bool isCapitalized(const string_t&);
    bool isQuoted(const string_t&);
    bool isQuoted(const string_t&, uchar32 inQuote);
    bool isDelimited(const string_t&, uchar32 inLeft, uchar32 inRight);

    uchar32 firstChar(const string_t&);
    uchar32 lastChar(const string_t&);

    bool startsWith(const string_t&, string_t const& inStartStr);
    bool endsWith(const string_t&, string_t const& inEndStr);

    ///@name Comparison
    s32  compare(const string_t& inLHS, const string_t& inRHS);
    bool isEqual(const string_t& inLHS, const string_t& inRHS);
    bool contains(const string_t& inStr, uchar32 inContains);
    bool contains(const string_t& inStr, const string_t& inContains);

    s32 format(string_t&, string_t const& formatString, const va_list_t& args);
    s32 formatAdd(string_t&, string_t const& formatString, const va_list_t& args);

    void upper(string_t& inStr);
    void lower(string_t& inStr);
    void capitalize(string_t& inStr);
    void capitalize(string_t& inStr, string_t const& inSeperators);

    string_t selectUntil(const string_t& inStr, uchar32 inFind);
    string_t selectUntil(const string_t& inStr, const string_t& inFind);

    string_t selectUntilLast(const string_t& inStr, uchar32 inFind);
    string_t selectUntilLast(const string_t& inStr, const string_t& inFind);

    string_t selectUntilIncluded(const string_t& str, const string_t& selection);

    string_t selectUntilEndExcludeSelection(const string_t& str, const string_t& selection);
    string_t selectUntilEndIncludeSelection(const string_t& str, const string_t& selection);

    ///@name Search/replace
    string_t find(string_t& inStr, uchar32 inFind);
    string_t find(string_t& inStr, const char* inFind);
    string_t find(string_t& inStr, const string_t& inFind);
    string_t findLast(string_t const& inStr, const string_t& inFind);
    string_t findOneOf(string_t& inStr, const string_t& inFind);
    string_t findOneOfLast(string_t& inStr, const string_t& inFind);

    void insert(string_t&, string_t const& pos, string_t const& insert);
    void insert_after(string_t&, string_t const& pos, string_t const& insert);

    void remove(string_t&, string_t const& selection);
    void find_remove(string_t&, const string_t& find);
    void find_replace(string_t&, string_t const& find, string_t const& replace);
    void remove_any(string_t&, const string_t& inAny);
    void replace_any(string_t&, string_t const& inAny, uchar32 inWith);

    void trim(string_t&);
    void trimLeft(string_t&);
    void trimRight(string_t&);
    void trim(string_t&, uchar32 inChar);
    void trimLeft(string_t&, uchar32 inChar);
    void trimRight(string_t&, uchar32 inChar);
    void trim(string_t&, string_t const& inCharSet);
    void trimLeft(string_t&, string_t const& inCharSet);
    void trimRight(string_t&, string_t const& inCharSet);
    void trimQuotes(string_t&);
    void trimQuotes(string_t&, uchar32 quote);
    void trimDelimiters(string_t&, uchar32 inLeft, uchar32 inRight);

    void reverse(string_t&);

    bool splitOn(string_t&, uchar32 inChar, string_t& outLeft, string_t& outRight);
    bool splitOn(string_t&, string_t& inStr, string_t& outLeft, string_t& outRight);
    bool splitOnLast(string_t&, uchar32 inChar, string_t& outLeft, string_t& outRight);
    bool splitOnLast(string_t&, string_t& inStr, string_t& outLeft, string_t& outRight);

    void concatenate(string_t& str, const string_t& con);
    void concatenate_repeat(string_t&, string_t const& con, s32 ntimes);

} // namespace ncore

#endif ///< __CSTRING_STRING_H__