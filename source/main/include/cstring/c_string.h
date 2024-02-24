#ifndef __CSTRING_STRING_H__
#define __CSTRING_STRING_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "ccore/c_debug.h"
#include "cbase/c_runes.h"

namespace ncore
{
    class va_t;
    class va_list_t;
    class alloc_t;
    class arena_t;

    class string_t
    {
    public:
        struct instance_t;
        struct data_t;
        struct range_t;

        string_t();
        string_t(arena_t* arena);
        string_t(arena_t* arena, const char* str);
        string_t(arena_t* arena, s32 _len);
        string_t(arena_t* arena, const string_t& other);
        string_t(arena_t* arena, const string_t& other, const string_t& concat);
        ~string_t();

        s32 format(const string_t& format, const va_t* argv, s32 argc);
        s32 formatAdd(const string_t& format, const va_t* argv, s32 argc);

        s32  cap() const;
        s32  size() const;
        string_t clone() const;
        bool is_slice() const;
        bool is_empty() const;

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

    //protected:
        string_t(instance_t* item);
        string_t(range_t range, instance_t* item);
        friend class string_unprotected_t;
        
        void release();

        mutable instance_t* m_item;
    };

    bool isUpper(const string_t&);
    bool isLower(const string_t&);
    bool isCapitalized(const string_t&);
    bool isQuoted(const string_t&);
    bool isQuoted(const string_t&, uchar32 inQuote);
    bool isDelimited(const string_t&, uchar32 inLeft, uchar32 inRight);

    uchar32 firstChar(const string_t&);
    uchar32 lastChar(const string_t&);

    bool startsWith(const string_t&, const string_t& inStartStr);
    bool endsWith(const string_t&, const string_t& inEndStr);

    ///@name Comparison
    s32  compare(const string_t& inLHS, const string_t& inRHS);
    bool isEqual(const string_t& inLHS, const string_t& inRHS);
    bool contains(const string_t& inStr, uchar32 inContains);
    bool contains(const string_t& inStr, const string_t& inContains);

    s32 format(string_t&, const string_t& formatString, const va_list_t& args);
    s32 formatAdd(string_t&, const string_t& formatString, const va_list_t& args);

    void upper(string_t& inStr);
    void lower(string_t& inStr);
    void capitalize(string_t& inStr);
    void capitalize(string_t& inStr, const string_t& inSeperators);

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
    string_t findLast(const string_t& inStr, const string_t& inFind);
    string_t findOneOf(string_t& inStr, const string_t& inFind);
    string_t findOneOfLast(string_t& inStr, const string_t& inFind);

    void insert(string_t&, const string_t& pos, const string_t& insert);
    void insert_after(string_t&, const string_t& pos, const string_t& insert);

    void remove(string_t&, const string_t& selection);
    void find_remove(string_t&, const string_t& find);
    void find_replace(string_t&, const string_t& find, const string_t& replace);
    void remove_any(string_t&, const string_t& inAny);
    void replace_any(string_t&, const string_t& inAny, uchar32 inWith);

    void trim(string_t&);
    void trimLeft(string_t&);
    void trimRight(string_t&);
    void trim(string_t&, uchar32 inChar);
    void trimLeft(string_t&, uchar32 inChar);
    void trimRight(string_t&, uchar32 inChar);
    void trim(string_t&, const string_t& inCharSet);
    void trimLeft(string_t&, const string_t& inCharSet);
    void trimRight(string_t&, const string_t& inCharSet);
    void trimQuotes(string_t&);
    void trimQuotes(string_t&, uchar32 quote);
    void trimDelimiters(string_t&, uchar32 inLeft, uchar32 inRight);

    void reverse(string_t&);

    bool splitOn(string_t&, uchar32 inChar, string_t& outLeft, string_t& outRight);
    bool splitOn(string_t&, string_t& inStr, string_t& outLeft, string_t& outRight);
    bool splitOnLast(string_t&, uchar32 inChar, string_t& outLeft, string_t& outRight);
    bool splitOnLast(string_t&, string_t& inStr, string_t& outLeft, string_t& outRight);

    void concatenate(string_t& str, const string_t& con);
    void concatenate_repeat(string_t&, const string_t& con, s32 ntimes);

}  // namespace ncore

#endif  ///< __CSTRING_STRING_H__