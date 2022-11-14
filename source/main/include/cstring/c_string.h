#ifndef __CSTRING_STRING_H__
#define __CSTRING_STRING_H__
#include "cbase/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "cbase/c_debug.h"
#include "cbase/c_runes.h"

// TODO needs to be refactored
// Plan:
// string_t should have no functions that can mutate it
// Instead you should get a slice from it and then use that to mutate.
// Slices can only be created from string_t and other slices.

namespace ncore
{
    class va_t;
    class va_list_t;
    class alloc_t;

    struct str_data_t;

    struct str_range_t
    {
        inline str_range_t(s32 f, s32 t) : from(f), to(t) {}
        s32  size() const { return to - from; }
        void reset() { *this = str_range_t(0, 0); }
        s32  from;
        s32  to;
    };

    class str_slice_t
    {
    public:
        void clear();
        void invalidate();

        bool is_empty() const;
        s32  cap() const;
        s32  size() const;

        s32 format(str_slice_t const& format, const va_list_t& args);
        s32 formatAdd(str_slice_t const& format, const va_list_t& args);

        str_slice_t  operator()(s32 to);
        str_slice_t  operator()(s32 from, s32 to);
        str_slice_t  operator()(s32 to) const;
        str_slice_t  operator()(s32 from, s32 to) const;
        uchar32      operator[](s32 index) const;
        str_slice_t& operator=(const char* other);
        str_slice_t& operator=(const str_slice_t& other);
        str_slice_t& operator+=(const str_slice_t& other);

        bool operator==(const str_slice_t& other) const;
        bool operator!=(const str_slice_t& other) const;

    protected:
        friend class string_t;
        friend class ustring_t;

        str_slice_t();
        str_slice_t(const str_slice_t& other);

        // You should only be able to work with them in a function but not construct them on the heap
        XCORE_CLASS_PLACEMENT_NEW_DELETE

        void add_to_list(str_slice_t const* s);
        void rem_from_list();

        void attach(str_slice_t& str);
        void clone(str_slice_t const& str);
        void release();

        str_data_t*  m_data;
        str_slice_t* m_next;
        str_slice_t* m_prev;
        str_range_t  m_view;
    };

    class string_t
    {
    public:
        string_t();
        //@TODO: We should also add wchar_t (utf16)
        string_t(const char* str);
        string_t(s32 _len, s32 _type);
        string_t(str_slice_t const& other); // copy constructor
        string_t(str_slice_t const& other, str_slice_t const& concat);
        ~string_t();

        s32      cap() const;
        s32      size() const;
        string_t clone() const;
        bool     is_empty() const;

        void        clear();
        str_slice_t slice() const;

        str_slice_t operator()(s32 from) const;
        str_slice_t operator()(s32 from, s32 to) const;
        uchar32     operator[](s32 index) const;
        bool        operator==(const string_t& other) const;
        bool        operator!=(const string_t& other) const;

    protected:
        friend class ustring_t;

        mutable str_data_t* m_data;
    };

    bool isUpper(const str_slice_t&);
    bool isLower(const str_slice_t&);
    bool isCapitalized(const str_slice_t&);
    bool isQuoted(const str_slice_t&);
    bool isQuoted(const str_slice_t&, uchar32 inQuote);
    bool isDelimited(const str_slice_t&, uchar32 inLeft, uchar32 inRight);

    uchar32 firstChar(const str_slice_t&);
    uchar32 lastChar(const str_slice_t&);

    bool startsWith(const str_slice_t&, str_slice_t const& inStartStr);
    bool endsWith(const str_slice_t&, str_slice_t const& inEndStr);

    ///@name Comparison
    s32  compare(const str_slice_t& inLHS, const str_slice_t& inRHS);
    bool isEqual(const str_slice_t& inLHS, const str_slice_t& inRHS);
    bool contains(const str_slice_t& inStr, uchar32 inContains);
    bool contains(const str_slice_t& inStr, const str_slice_t& inContains);

    s32 format(str_slice_t&, str_slice_t const& formatString, const va_list_t& args);
    s32 formatAdd(str_slice_t&, str_slice_t const& formatString, const va_list_t& args);

    void upper(str_slice_t& inStr);
    void lower(str_slice_t& inStr);
    void capitalize(str_slice_t& inStr);
    void capitalize(str_slice_t& inStr, str_slice_t const& inSeperators);

    str_slice_t selectUntil(const str_slice_t& inStr, uchar32 inFind);
    str_slice_t selectUntil(const str_slice_t& inStr, const str_slice_t& inFind);

    str_slice_t selectUntilLast(const str_slice_t& inStr, uchar32 inFind);
    str_slice_t selectUntilLast(const str_slice_t& inStr, const str_slice_t& inFind);

    str_slice_t selectUntilIncluded(const str_slice_t& str, const str_slice_t& selection);

    str_slice_t selectUntilEndExcludeSelection(const str_slice_t& str, const str_slice_t& selection);
    str_slice_t selectUntilEndIncludeSelection(const str_slice_t& str, const str_slice_t& selection);

    ///@name Search/replace
    str_slice_t find(str_slice_t& inStr, uchar32 inFind);
    str_slice_t find(str_slice_t& inStr, const char* inFind);
    str_slice_t find(str_slice_t& inStr, const str_slice_t& inFind);
    str_slice_t findLast(str_slice_t const& inStr, const str_slice_t& inFind);
    str_slice_t findOneOf(str_slice_t& inStr, const str_slice_t& inFind);
    str_slice_t findOneOfLast(str_slice_t& inStr, const str_slice_t& inFind);

    void insert(str_slice_t&, str_slice_t const& pos, str_slice_t const& insert);
    void insert_after(str_slice_t&, str_slice_t const& pos, str_slice_t const& insert);

    void remove(str_slice_t&, str_slice_t const& selection);
    void find_remove(str_slice_t&, const str_slice_t& find);
    void find_replace(str_slice_t&, str_slice_t const& find, str_slice_t const& replace);
    void remove_any(str_slice_t&, const str_slice_t& inAny);
    void replace_any(str_slice_t&, str_slice_t const& inAny, uchar32 inWith);

    void trim(str_slice_t&);
    void trimLeft(str_slice_t&);
    void trimRight(str_slice_t&);
    void trim(str_slice_t&, uchar32 inChar);
    void trimLeft(str_slice_t&, uchar32 inChar);
    void trimRight(str_slice_t&, uchar32 inChar);
    void trim(str_slice_t&, str_slice_t const& inCharSet);
    void trimLeft(str_slice_t&, str_slice_t const& inCharSet);
    void trimRight(str_slice_t&, str_slice_t const& inCharSet);
    void trimQuotes(str_slice_t&);
    void trimQuotes(str_slice_t&, uchar32 quote);
    void trimDelimiters(str_slice_t&, uchar32 inLeft, uchar32 inRight);

    void reverse(str_slice_t&);

    bool splitOn(str_slice_t&, uchar32 inChar, str_slice_t& outLeft, str_slice_t& outRight);
    bool splitOn(str_slice_t&, str_slice_t& inStr, str_slice_t& outLeft, str_slice_t& outRight);
    bool splitOnLast(str_slice_t&, uchar32 inChar, str_slice_t& outLeft, str_slice_t& outRight);
    bool splitOnLast(str_slice_t&, str_slice_t& inStr, str_slice_t& outLeft, str_slice_t& outRight);

    void concatenate(str_slice_t& str, const str_slice_t& con);
    void concatenate_repeat(str_slice_t&, str_slice_t const& con, s32 ntimes);

} // namespace ncore

#endif ///< __CSTRING_STRING_H__