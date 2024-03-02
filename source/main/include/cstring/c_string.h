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
        string_t(const char* str);
        string_t(s32 _len);
        string_t(const string_t& other);
        string_t(const string_t& other, const string_t& concat);
        ~string_t();

        s32 format(const string_t& format, const va_t* argv, s32 argc);
        s32 formatAdd(const string_t& format, const va_t* argv, s32 argc);

        s32      cap() const;
        s32      size() const;
        bool     is_slice() const;
        bool     is_empty() const;

        void     clear();
        string_t slice() const;
        string_t clone() const;

        string_t operator()(s32 from, s32 to) const;
        uchar32  operator[](s32 index) const;

        string_t& operator=(const char* other);
        string_t& operator=(const string_t& other);
        string_t& operator+=(const string_t& other);

        bool operator==(const string_t& other) const;
        bool operator!=(const string_t& other) const;

        bool isUpper() const;
        bool isLower() const;
        bool isCapitalized() const;
        bool isQuoted() const;
        bool isQuoted(uchar32 quote) const;
        bool isDelimited(uchar32 left, uchar32 right) const;

        uchar32 firstChar() const;
        uchar32 lastChar() const;

        bool startsWith(const string_t& startStr) const;
        bool endsWith(const string_t& endStr) const;

        ///@name Comparison
        s32  compare(const string_t& rhs) const;
        bool isEqual(const string_t& rhs) const;
        bool contains(uchar32 contains) const;
        bool contains(const string_t& contains) const;

        s32 format(const string_t& format, const va_list_t& args);
        s32 formatAdd(const string_t& format, const va_list_t& args);

        void toUpper();
        void toLower();
        void capitalize();
        void capitalize(const string_t& seperators);

        // select
        string_t select(u32 from, u32 to) const;
        string_t selectUntil(uchar32 find) const;
        string_t selectUntil(const string_t& find) const;

        string_t selectUntilLast(uchar32 find) const;
        string_t selectUntilLast(const string_t& find) const;

        string_t selectUntilIncluded(uchar32 find) const;
        string_t selectUntilIncluded(const string_t& selection) const;

        string_t selectUntilEndExcludeSelection(const string_t& selection) const;
        string_t selectUntilEndIncludeSelection(const string_t& selection) const;

        void removeSelection(const string_t& selection);

        // Search/Replace
        string_t find(uchar32 find) const;
        string_t find(const char* find) const;
        string_t find(const string_t& find) const;
        string_t findLast(const string_t& find) const;
        string_t findOneOf(const string_t& find) const;
        string_t findOneOfLast(const string_t& find) const;
        s32      findRemove(const string_t& find, s32 ntimes = 1);
        s32      findReplace(const string_t& find, const string_t& replace, s32 ntimes = 1);

        void insertReplace(const string_t& pos, const string_t& insert);
        void insertBefore(const string_t& pos, const string_t& insert);
        void insertAfter(const string_t& pos, const string_t& insert);

        s32 remove(uchar32 c, s32 ntimes = 0);
        s32 removeAny(const string_t& any, s32 ntimes = 0);
        s32 replaceAny(const string_t& any, uchar32 with, s32 ntimes = 0);

        // Trimming
        void trim();
        void trimLeft();
        void trimRight();
        void trim(uchar32 c);
        void trimLeft(uchar32 c);
        void trimRight(uchar32 c);
        void trim(const string_t& charSet);
        void trimLeft(const string_t& charSet);
        void trimRight(const string_t& charSet);
        void trimQuotes();
        void trimQuotes(uchar32 quote);
        void trimDelimiters(uchar32 left, uchar32 right);

        void reverse();

        bool selectBeforeAndAfter(const string_t& selection, string_t& outBefore, string_t& outAfter) const;
        bool findCharSelectBeforeAndAfter(uchar32 find, string_t& outBefore, string_t& outAfter) const;
        bool findStrSelectBeforeAndAfter(const string_t& find, string_t& outBefore, string_t& outAfter) const;
        bool findCharLastSelectBeforeAndAfter(uchar32 find, string_t& outBefore, string_t& outAfter) const;
        bool findStrLastSelectBeforeAndAfter(const string_t& find, string_t& outBefore, string_t& outAfter) const;

        void concatenate(const string_t& con);
        void concatenate(const string_t& strA, const string_t& strB);
        void concatenate_repeat(const string_t& con, s32 ntimes);

        // protected:
        string_t(instance_t* item);
        string_t(instance_t* item, range_t const& range);
        friend class string_unprotected_t;

        void release();

        mutable instance_t* m_item;
    };

}  // namespace ncore

#endif  ///< __CSTRING_STRING_H__