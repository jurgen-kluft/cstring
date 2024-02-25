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

        struct range_t
        {
            u32 m_from;
            u32 m_to;

            inline s64  get_length() const { return m_to - m_from; }
            inline bool is_empty() const { return m_from == m_to; }
            inline bool is_inside(range_t const& parent) const { return m_from >= parent.m_from && m_to <= parent.m_to; }
        };

        string_t();
        string_t(arena_t* arena);
        string_t(arena_t* arena, const char* str);
        string_t(arena_t* arena, s32 _len);
        string_t(arena_t* arena, const string_t& other);
        string_t(arena_t* arena, const string_t& other, const string_t& concat);
        ~string_t();

        s32 format(const string_t& format, const va_t* argv, s32 argc);
        s32 formatAdd(const string_t& format, const va_t* argv, s32 argc);

        s32      cap() const;
        s32      size() const;
        string_t clone() const;
        bool     is_slice() const;
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

        string_t selectUntilIncluded(const string_t& selection) const;

        string_t selectUntilEndExcludeSelection(const string_t& selection) const;
        string_t selectUntilEndIncludeSelection(const string_t& selection) const;

        void remove_selection(const string_t& selection);

        // Search/Replace
        string_t find(uchar32 find) const;
        string_t find(const char* find) const;
        string_t find(const string_t& find) const;
        string_t findLast(const string_t& find) const;
        string_t findOneOf(const string_t& find) const;
        string_t findOneOfLast(const string_t& find) const;
        s32      find_remove(const string_t& find, s32 ntimes = 1);
        s32      find_replace(const string_t& find, const string_t& replace, s32 ntimes = 1);

        void insert_replace(const string_t& pos, const string_t& insert);
        void insert_before(const string_t& pos, const string_t& insert);
        void insert_after(const string_t& pos, const string_t& insert);

        void remove(uchar32 c, s32 ntimes = 0);
        s32  remove_any(const string_t& any, s32 ntimes = 0);
        s32  replace_any(const string_t& any, uchar32 with, s32 ntimes = 0);

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

        bool selectBeforeAndAfter(uchar32 c, string_t& outBefore, string_t& outAfter) const;
        bool selectBeforeAndAfter(const string_t& str, string_t& outBefore, string_t& outAfter) const;
        bool selectBeforeAndAfterLast(uchar32 c, string_t& outBefore, string_t& outAfter) const;
        bool selectBeforeAndAfterLast(const string_t& str, string_t& outBefore, string_t& outAfter) const;

        void concatenate(const string_t& con);
        void concatenate_repeat(const string_t& con, s32 ntimes);

        // protected:
        string_t(instance_t* item);
        string_t(range_t range, instance_t* item);
        friend class string_unprotected_t;

        void release();

        mutable instance_t* m_item;
    };

}  // namespace ncore

#endif  ///< __CSTRING_STRING_H__