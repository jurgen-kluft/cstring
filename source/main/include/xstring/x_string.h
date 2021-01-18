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
// string_t, a UTF32 string class
//==============================================================================
namespace xcore
{
    class va_t;
    class va_list_t;
    class alloc_t;

    class string_t
    {
    protected:
        struct data;
        struct range
        {
            inline range() : from(0), to(0) {}
            inline range(s32 f, s32 t) : from(f), to(t) {}
            inline s32 size() const { return to - from; }
            s32        from, to;
        };

    public:
        string_t();
        string_t(runes_alloc_t* allocator);
        string_t(runes_alloc_t* allocator, const char* str);
        string_t(const char* str);
        string_t(string_t const& other);
        ~string_t();

        struct view
        {
            view(const view& other);
            ~view();

            s32     size() const;
            bool    is_empty() const;
            string_t to_string() const;

            view operator()(s32 to);
            view operator()(s32 from, s32 to);
            view operator()(s32 to) const;
            view operator()(s32 from, s32 to) const;

            uchar32 operator[](s32 index) const;

            view& operator=(view const& other);

            bool operator==(view const& other) const;
            bool operator!=(view const& other) const;

        protected:
            friend class string_t;
            friend class xview;
            view(string_t::data*);

            void     add();
            void     rem();
            void     invalidate();
            crunes_t get_runes() const;

            string_t::data* m_data;
            range          m_view;
            view*          m_next;
            view*          m_prev;
        };

        string_t(string_t::view const& left, string_t::view const& right);

        bool is_empty() const;
        s32  cap() const;
        s32  size() const;

        void clear();

        view full();
        view full() const;

        view operator()(s32 to);
        view operator()(s32 from, s32 to);

        view operator()(s32 to) const;
        view operator()(s32 from, s32 to) const;

        uchar32 operator[](s32 index) const;

        string_t& operator=(const string_t& other);
        string_t& operator=(const string_t::view& other);

        bool operator==(const string_t& other) const;
        bool operator!=(const string_t& other) const;

        operator view() { return full(); }

        static runes_alloc_t* s_allocator;

    protected:
        friend struct view;
        friend class xview;

        string_t(runes_alloc_t* mem, s32 size);

        void release();
        void clone(runes_t const& str, runes_alloc_t* allocator);

        struct data
        {
            inline data() : m_alloc(nullptr), m_runes(), m_views(nullptr) {}
            inline data(runes_alloc_t* a) : m_alloc(a), m_runes(), m_views(nullptr) {}
            runes_alloc_t* m_alloc;
            runes_t        m_runes;
            mutable view*  m_views;
        };
        mutable data m_data;
    };

    bool isUpper(const string_t::view&);
    bool isLower(const string_t::view&);
    bool isCapitalized(const string_t::view&);
    bool isQuoted(const string_t::view&);
    bool isQuoted(const string_t::view&, uchar32 inQuote);
    bool isDelimited(const string_t::view&, uchar32 inLeft, uchar32 inRight);

    uchar32 firstChar(const string_t::view&);
    uchar32 lastChar(const string_t::view&);

    bool startsWith(const string_t::view&, string_t::view const& inStartStr);
    bool endsWith(const string_t::view&, string_t::view const& inEndStr);

    ///@name Comparison
    s32  compare(const string_t::view& inLHS, const string_t::view& inRHS);
    bool isEqual(const string_t::view& inLHS, const string_t::view& inRHS);
    bool contains(const string_t::view& inStr, uchar32 inContains);
    bool contains(const string_t::view& inStr, const string_t::view& inContains);

    s32 format(string_t&, string_t::view const& formatString, const va_list_t& args);
    s32 formatAdd(string_t&, string_t::view const& formatString, const va_list_t& args);

    void upper(string_t::view& inStr);
    void lower(string_t::view& inStr);
    void capitalize(string_t::view& inStr);
    void capitalize(string_t::view& inStr, string_t::view const& inSeperators);

    string_t::view selectUntil(const string_t::view& inStr, uchar32 inFind);
    string_t::view selectUntil(const string_t::view& inStr, const string_t::view& inFind);

    string_t::view selectUntilLast(const string_t::view& inStr, uchar32 inFind);
    string_t::view selectUntilLast(const string_t::view& inStr, const string_t::view& inFind);

    string_t::view selectUntilIncluded(const string_t::view& str, const string_t::view& selection);

    string_t::view selectUntilEndExcludeSelection(const string_t::view& str, const string_t::view& selection);
    string_t::view selectUntilEndIncludeSelection(const string_t::view& str, const string_t::view& selection);

    ///@name Search/replace
    string_t::view find(string_t& inStr, uchar32 inFind);
    string_t::view find(string_t::view& inStr, uchar32 inFind);
    string_t::view find(string_t& inStr, const string_t::view& inFind);
    string_t::view find(string_t::view& inStr, const string_t::view& inFind);
    string_t::view findLast(string_t::view& inStr, const string_t::view& inFind);
    string_t::view findOneOf(string_t::view& inStr, const string_t::view& inFind);
    string_t::view findOneOfLast(string_t::view& inStr, const string_t::view& inFind);

    void insert(string_t&, string_t::view const& pos, string_t::view const& insert);
    void insert_after(string_t&, string_t::view const& pos, string_t::view const& insert);

    void remove(string_t&, string_t::view const& selection);
    void find_remove(string_t&, const string_t::view& find);
    void find_replace(string_t&, string_t::view const& find, string_t::view const& replace);
    void remove_any(string_t&, const string_t::view& inAny);
    void replace_any(string_t::view&, string_t::view const& inAny, uchar32 inWith);

    void trim(string_t::view&);
    void trimLeft(string_t::view&);
    void trimRight(string_t::view&);
    void trim(string_t::view&, uchar32 inChar);
    void trimLeft(string_t::view&, uchar32 inChar);
    void trimRight(string_t::view&, uchar32 inChar);
    void trim(string_t::view&, string_t::view const& inCharSet);
    void trimLeft(string_t::view&, string_t::view const& inCharSet);
    void trimRight(string_t::view&, string_t::view const& inCharSet);
    void trimQuotes(string_t::view&);
    void trimQuotes(string_t::view&, uchar32 quote);
    void trimDelimiters(string_t::view&, uchar32 inLeft, uchar32 inRight);

    void reverse(string_t::view&);

    bool splitOn(string_t::view&, uchar32 inChar, string_t::view& outLeft, string_t::view& outRight);
    bool splitOn(string_t::view&, string_t::view& inStr, string_t::view& outLeft, string_t::view& outRight);
    bool splitOnLast(string_t::view&, uchar32 inChar, string_t::view& outLeft, string_t::view& outRight);
    bool splitOnLast(string_t::view&, string_t::view& inStr, string_t::view& outLeft, string_t::view& outRight);

    void concatenate(string_t& str, const string_t::view& con);
    void concatenate_repeat(string_t&, string_t::view const& con, s32 ntimes);

    // Global string_t operators

    inline string_t operator+(const string_t& left, const string_t& right) { return string_t(left.full(), right.full()); }
} // namespace xcore

#endif ///< __XSTRING_STRING_H__